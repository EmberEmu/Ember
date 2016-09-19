/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginHandler.h"
#include "Patcher.h"
#include "grunt/Packets.h"
#include <boost/range/adaptor/map.hpp>
#include <stdexcept>
#include <utility>
 
namespace ember {

bool LoginHandler::update_state(const grunt::Packet* packet) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	State prev_state = state_;
	state_ = State::CLOSED;

	switch(prev_state) {
		case State::INITIAL_CHALLENGE:
			initiate_login(packet);
			break;
		case State::LOGIN_PROOF:
			handle_login_proof(packet);
			break;
		case State::RECONNECT_PROOF:
			handle_reconnect_proof(packet);
			break;
		case State::REQUEST_REALMS:
			send_realm_list(packet);
			break;
		case State::SURVEY_INITIATE:
			handle_transfer_ack(packet, true);
			break;
		case State::PATCH_INITIATE:
			handle_transfer_ack(packet, false);
			break;
		case State::SURVEY_TRANSFER:
		case State::PATCH_TRANSFER:
			handle_transfer_abort();
			break;
		case State::SURVEY_RESULT:
			handle_survey_result(packet);
			break;
		case State::CLOSED:
			return false;
		default:
			LOG_DEBUG(logger_) << "Received packet out of sync" << LOG_ASYNC;
			return false;
	}

	return true;
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	state_ = State::CLOSED;
	return false;
}

bool LoginHandler::update_state(std::shared_ptr<Action> action) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	State prev_state = state_;
	state_ = State::CLOSED;

	switch(prev_state) {
		case State::FETCHING_USER_LOGIN:
			send_login_challenge(static_cast<FetchUserAction*>(action.get()));
			break;
		case State::FETCHING_USER_RECONNECT:
			fetch_session_key(static_cast<FetchUserAction*>(action.get()));
			break;
		case State::FETCHING_SESSION:
			send_reconnect_challenge(static_cast<FetchSessionKeyAction*>(action.get()));
			break;
		case State::WRITING_SESSION:
			on_session_write(static_cast<RegisterSessionAction*>(action.get()));
			break;
		case State::REQUEST_REALMS:
			on_survey_write(static_cast<SaveSurveyAction*>(action.get()));
			break;
		case State::FETCHING_CHARACTER_DATA:
			on_character_data(static_cast<FetchCharacterCounts*>(action.get()));
			break;
		case State::CLOSED:
			return false;
		default:
			LOG_WARN(logger_) << "Received action out of sync" << LOG_ASYNC;
			return false;
	}

	return true;
} catch(std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	state_ = State::CLOSED;
	return false;
}

void LoginHandler::initiate_login(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto challenge = dynamic_cast<const grunt::client::LoginChallenge*>(packet);

	if(!challenge) {
		throw std::runtime_error("Expected CMD_LOGIN/RECONNECT_CHALLENGE");
	}

	if(challenge->opcode == grunt::Opcode::CMD_AUTH_LOGON_CHALLENGE
	   && challenge->protocol_ver != CONNECT_PROTO_VERSION
	   || challenge->opcode == grunt::Opcode::CMD_AUTH_RECONNECT_CHALLENGE
	   && challenge->protocol_ver != RECONNECT_PROTO_VERSION) {
		LOG_DEBUG(logger_) << "Unsupported protocol version, "
		                   << challenge->protocol_ver << LOG_ASYNC;
		return;
	}

	if(challenge->game != grunt::Game::WoW) {
		LOG_DEBUG(logger_) << "Bad game magic from client"  << LOG_ASYNC;
		return;
	}

	LOG_DEBUG(logger_) << "Challenge: " << challenge->username << ", "
	                   << challenge->version << ", " << source_ << LOG_ASYNC;

	challenge_ = *challenge;

	Patcher::PatchLevel patch_level = patcher_.check_version(challenge->version);

	switch(patch_level) {
		case Patcher::PatchLevel::OK:
			fetch_user(challenge->opcode, challenge->username);
			break;
		case Patcher::PatchLevel::TOO_NEW:
			reject_client(challenge->version);
			break;
		case Patcher::PatchLevel::TOO_OLD:
			patch_client(challenge);
			break;
	}
}

void LoginHandler::fetch_user(grunt::Opcode opcode, const std::string& username) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(opcode) {
		case grunt::Opcode::CMD_AUTH_LOGON_CHALLENGE:
			state_ = State::FETCHING_USER_LOGIN;
			break;
		case grunt::Opcode::CMD_AUTH_RECONNECT_CHALLENGE:
			state_ = State::FETCHING_USER_RECONNECT;
			break;
		default:
			state_ = State::CLOSED;
			BOOST_ASSERT_MSG(false, "Impossible fetch_user condition");
	}

	auto action = std::make_shared<FetchUserAction>(username, user_src_);
	execute_async(action);
}

void LoginHandler::fetch_session_key(FetchUserAction* action_res) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!(user_ = action_res->get_result())) {
		LOG_DEBUG(logger_) << "Account not found: " << action_res->username() << LOG_ASYNC;
		return;
	}

	state_ = State::FETCHING_SESSION;
	auto action = std::make_shared<FetchSessionKeyAction>(acct_svc_, user_->id());
	execute_async(action);
}

void LoginHandler::reject_client(const GameVersion& version) {
	LOG_DEBUG(logger_) << "Rejecting client version " << version << LOG_ASYNC;

	grunt::server::LoginChallenge response;
	response.result = grunt::Result::FAIL_VERSION_INVALID;
	send(response);
}

void LoginHandler::build_login_challenge(grunt::server::LoginChallenge& packet) {	
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto values = login_auth_->challenge_reply();
	packet.B = values.B;
	packet.g_len = static_cast<std::uint8_t>(values.gen.generator().bytes());
	packet.g = static_cast<std::uint8_t>(values.gen.generator().to_u32bit());
	packet.n_len = grunt::server::LoginChallenge::PRIME_LENGTH;
	packet.N = values.gen.prime();
	packet.s = values.salt;
	packet.two_factor_auth = false;

	if(user_->pin_method() != PINMethod::NONE) {
		packet.two_factor_auth = true;
		packet.pin_grid_seed = pin_auth_.grid_seed();
		packet.pin_salt = pin_auth_.server_salt();
	}

	checksum_salt_ = Botan::AutoSeeded_RNG().random_vec(16);
	std::copy(checksum_salt_.begin(), checksum_salt_.end(), packet.checksum_salt.data());
}

void LoginHandler::send_login_challenge(FetchUserAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	grunt::server::LoginChallenge response;
	response.result = grunt::Result::SUCCESS;

	try {
		if((user_ = action->get_result())) {
			login_auth_ = std::make_unique<LoginAuthenticator>(*user_);
			build_login_challenge(response);
			state_ = State::LOGIN_PROOF;
		} else {
			// leaks information on whether the account exists (could send challenge anyway?)
			response.result = grunt::Result::FAIL_UNKNOWN_ACCOUNT;
			metrics_.increment("login_failure");
			LOG_DEBUG(logger_) << "Account not found: " << action->username() << LOG_ASYNC;
		}
	} catch(dal::exception& e) {
		response.result = grunt::Result::FAIL_DB_BUSY;
		metrics_.increment("login_internal_failure");
		LOG_ERROR(logger_) << "DAL failure for " << action->username()
		                   << ": " << e.what() << LOG_ASYNC;
	} catch(Botan::Exception& e) {
		response.result = grunt::Result::FAIL_DB_BUSY;
		metrics_.increment("login_internal_failure");
		LOG_ERROR(logger_) << "Encoding failure for " << action->username()
		                   << ": " << e.what() << LOG_ASYNC;
	}
	
	send(response);
}

void LoginHandler::send_reconnect_proof(grunt::Result result) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	LOG_DEBUG(logger_) << "Reconnect result for " << user_->username() << ": "
	                   << grunt::to_string(result) << LOG_ASYNC;

	if(result == grunt::Result::SUCCESS) {
		metrics_.increment("login_success");
	} else {
		metrics_.increment("login_failure");
	}

	grunt::server::ReconnectProof response;
	response.result = result;
	send(response);
}

void LoginHandler::send_reconnect_challenge(FetchSessionKeyAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	grunt::server::ReconnectChallenge response;
	response.result = grunt::Result::SUCCESS;

	checksum_salt_ = Botan::AutoSeeded_RNG().random_vec(response.salt.size());
	std::copy(checksum_salt_.begin(), checksum_salt_.end(), response.salt.data());

	auto res = action->get_result();

	if(res.first == messaging::account::Status::OK) {
		state_ = State::RECONNECT_PROOF;
		reconn_auth_ = std::make_unique<ReconnectAuthenticator>(user_->username(), res.second, checksum_salt_);
	} else if(res.first == messaging::account::Status::SESSION_NOT_FOUND) {
		metrics_.increment("login_failure");
		response.result = grunt::Result::FAIL_NOACCESS;
		LOG_DEBUG(logger_) << "Reconnect failed, session not found for "
		                   << user_->username() << LOG_ASYNC;
	} else {
		metrics_.increment("login_internal_failure");
		response.result = grunt::Result::FAIL_DB_BUSY;
		LOG_ERROR(logger_) << messaging::account::EnumNameStatus(res.first)
		                   << " from peer during reconnect challenge" << LOG_ASYNC;
	}
	
	send(response);
}

bool LoginHandler::validate_pin(const grunt::client::LoginProof* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	// no PIN was expected, nothing to validate
	if(user_->pin_method() == PINMethod::NONE) {
		return true;
	}

	// PIN auth is enabled for this user, make sure the packet has PIN data
	if(!packet->two_factor_auth) {
		return false;
	}

	bool result = false;

	pin_auth_.set_client_hash(packet->pin_hash);
	pin_auth_.set_client_salt(packet->pin_salt);

	if(user_->pin_method() == PINMethod::FIXED) {
		pin_auth_.set_pin(user_->pin());
		result = pin_auth_.validate_pin(packet->pin_hash);
	}

	if(user_->pin_method() == PINMethod::TOTP) {
		for(auto interval = -1; interval < 2; ++interval) { // try time intervals -1 to +1
			pin_auth_.set_pin(PINAuthenticator::generate_totp_pin(user_->totp_token(), interval));

			if(pin_auth_.validate_pin(packet->pin_hash)) {
				result = true;
				break;
			}
		}
	}

	LOG_DEBUG(logger_) << "PIN authentication for " << user_->username()
	                   << (result ? " OK" : " failed") << LOG_ASYNC;

	return result;
}

bool LoginHandler::validate_client_integrity(const std::array<std::uint8_t, HASH_LENGTH>& hash,
											 const Botan::BigInt& salt, bool reconnect) {
	auto decoded = Botan::BigInt::encode(salt);
	std::reverse(decoded.begin(), decoded.end());
	return validate_client_integrity(hash, decoded.begin(), decoded.size(), reconnect);
}

bool LoginHandler::validate_client_integrity(const std::array<std::uint8_t, HASH_LENGTH>& client_hash,
                                             const std::uint8_t* salt, std::size_t len,
                                             bool reconnect) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!exe_checksum_) {
		return true;
	}

	Botan::SecureVector<Botan::byte> hash;

	// client doesn't bother to checksum the binaries on reconnect, it just hashes the salt (=])
	if(reconnect) {
		Botan::SecureVector<Botan::byte> exe_checksum(HASH_LENGTH);
		hash = exe_checksum_->finalise(exe_checksum, salt, len);
	} else {
		hash = exe_checksum_->finalise(exe_checksum_->checksum(checksum_salt_), salt, len);
	}

	return std::equal(hash.begin(), hash.end(), client_hash.begin(), client_hash.end());
}

void LoginHandler::handle_login_proof(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto proof_packet = dynamic_cast<const grunt::client::LoginProof*>(packet);

	if(!proof_packet) {
		throw std::runtime_error("Expected CMD_AUTH_LOGIN_PROOF");
	}
	
	if(!validate_client_integrity(proof_packet->client_checksum, proof_packet->A, false)) {
		send_login_proof(grunt::Result::FAIL_VERSION_INVALID);
		return;
	}

	if(!validate_pin(proof_packet)) {
		send_login_proof(grunt::Result::FAIL_INCORRECT_PASSWORD);
		return;
	}

	auto proof = login_auth_->proof_check(proof_packet);
	auto result = grunt::Result::FAIL_INCORRECT_PASSWORD;
	
	if(proof.match) {
		if(user_->banned()) {
			result = grunt::Result::FAIL_BANNED;
		} else if(user_->suspended()) {
			result = grunt::Result::FAIL_SUSPENDED;
		} else if(!user_->subscriber()) {
			result = grunt::Result::FAIL_NO_TIME;
		/*} else if(parental_controls) {
			res = grunt::Result::FAIL_PARENTAL_CONTROLS;*/
		} else {
			result = grunt::Result::SUCCESS;
		}
	}

	if(result == grunt::Result::SUCCESS) {
		state_ = State::WRITING_SESSION;
		server_proof_ = proof.server_proof;

		auto action = std::make_shared<RegisterSessionAction>(
			acct_svc_, user_->id(),
		    login_auth_->session_key()
		);

		execute_async(action);
	} else {
		send_login_proof(result);
	}
}

void LoginHandler::send_login_proof(grunt::Result result, bool survey) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	grunt::server::LoginProof response;
	response.result = result;

	if(result == grunt::Result::SUCCESS) {
		metrics_.increment("login_success");
		response.M2 = server_proof_;
		response.survey_id = survey? patcher_.survey_id() : 0;
	} else {
		metrics_.increment("login_failure");
	}

	LOG_DEBUG(logger_) << "Login result for " << user_->username() << ": "
	                   << grunt::to_string(result) << LOG_ASYNC;

	send(response);
}


void LoginHandler::on_character_data(FetchCharacterCounts* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	try {
		char_count_ = action->get_result();
	} catch(dal::exception& e) { // not a fatal exception, we'll keep going without the data
		metrics_.increment("login_internal_failure");
		LOG_ERROR(logger_) << "DAL failure for " << user_->username()
			<< ": " << e.what() << LOG_ASYNC;
	}

	bool trigger_survey = false;
	state_ = State::REQUEST_REALMS;

	if(patcher_.survey_platform(challenge_.platform, challenge_.os)) {
		if(user_->survey_request() && !action->reconnect()) {
			state_ = State::SURVEY_INITIATE;
			trigger_survey = true;
		}
	}

	if(action->reconnect()) {
		send_reconnect_proof(grunt::Result::SUCCESS);
	} else {
		send_login_proof(grunt::Result::SUCCESS, trigger_survey);
	}

	if(trigger_survey) {
		initiate_file_transfer(patcher_.survey_meta());
	}
}

void LoginHandler::on_session_write(RegisterSessionAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto result = action->get_result();
	grunt::Result response = grunt::Result::SUCCESS;

	if(result == messaging::account::Status::OK) {
		state_ = State::FETCHING_CHARACTER_DATA;
	} else if(result == messaging::account::Status::ALREADY_LOGGED_IN) {
		response = grunt::Result::FAIL_ALREADY_ONLINE;
	} else {
		metrics_.increment("login_internal_failure");
		response = grunt::Result::FAIL_DB_BUSY;
		LOG_ERROR(logger_) << messaging::account::EnumNameStatus(result)
		                   << " from peer during login" << LOG_ASYNC;
	}

	// defer sending the response until we've fetched the character data
	if(result == messaging::account::Status::OK) {
		execute_async(std::make_shared<FetchCharacterCounts>(user_->id(), user_src_));
	} else {
		send_login_proof(response);
	}
}

void LoginHandler::handle_reconnect_proof(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto proof = dynamic_cast<const grunt::client::ReconnectProof*>(packet);

	if(!proof) {
		throw std::runtime_error("Expected CMD_AUTH_RECONNECT_PROOF");
	}
	
	if(!validate_client_integrity(proof->client_checksum, proof->salt.data(), proof->salt.size(), true)) {
		send_reconnect_proof(grunt::Result::FAIL_VERSION_INVALID);
		return;
	}

	if(reconn_auth_->proof_check(proof)) {
		state_ = State::FETCHING_CHARACTER_DATA;
		execute_async(std::make_shared<FetchCharacterCounts>(user_->id(), user_src_, true));
	} else {
		send_reconnect_proof(grunt::Result::FAIL_INCORRECT_PASSWORD);
	}

	grunt::server::ReconnectProof response;
	response.result = grunt::Result::SUCCESS;

	state_ = State::REQUEST_REALMS;
	send(response);
}

void LoginHandler::send_realm_list(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!dynamic_cast<const grunt::client::RequestRealmList*>(packet)) {
		throw std::runtime_error("Expected CMD_REALM_LIST");
	}

	grunt::server::RealmList response;

	std::shared_ptr<const RealmMap> realms = realm_list_.realms();

	for(auto& realm : *realms | boost::adaptors::map_values) {
		response.realms.push_back({ realm, char_count_[realm.id] });
	}

	state_ = State::REQUEST_REALMS;
	send(response);
}

void LoginHandler::patch_client(const grunt::client::LoginChallenge* challenge) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto meta = patcher_.find_patch(challenge->version, challenge->locale,
	                                challenge->platform, challenge->os);

	if(!meta) {
		reject_client(challenge->version);
		return;
	}
	
	std::ifstream patch(meta->file_meta.path + meta->file_meta.name);

	if(!patch.is_open()) {
		LOG_ERROR(logger_) << "Could not open patch, " << meta->file_meta.name << LOG_ASYNC;
	}

	transfer_state_.file = std::move(patch);
	initiate_file_transfer(meta->file_meta);
}

void LoginHandler::initiate_file_transfer(const FileMeta& meta) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	LOG_DEBUG(logger_) << "Initiating transfer of " << meta.name << " to "
     	               << user_->username() << LOG_ASYNC;

	transfer_state_.size = meta.size;

	grunt::server::TransferInitiate response;
	response.filename = meta.name;
	response.filesize = meta.size;
	response.md5 = meta.md5;
	send(response);
}

void LoginHandler::handle_survey_result(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto survey = dynamic_cast<const grunt::client::SurveyResult*>(packet);

	if(!survey) {
		throw std::runtime_error("Expected CMD_SURVEY_RESULT");
	}

	// allow the client to request the realmlist without waiting on the survey write callback
	state_ = State::REQUEST_REALMS;

	if(survey->survey_id != patcher_.survey_id()) {
		LOG_DEBUG(logger_) << "Received an invalid survey ID from "
		                   << user_->username() << LOG_ASYNC;
		return;
	}

	// this can be caused by the client having already sent data for the active survey ID
	if(survey->error) {
		return;
	}

	auto action = std::make_shared<SaveSurveyAction>(
		user_->id(), user_src_,
		survey->survey_id, survey->data
	);

	metrics_.increment("surveys_received");
	execute_async(action);
}

void LoginHandler::on_survey_write(SaveSurveyAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(action->error()) {
		LOG_ERROR(logger_) << "DAL failure for " << user_->username()
		                   << ", " << action->exception().what() << LOG_ASYNC;
	}
}

void LoginHandler::handle_transfer_ack(const grunt::Packet* packet, bool survey) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(packet->opcode) {
		case grunt::Opcode::CMD_XFER_RESUME:
			transfer_state_.offset = static_cast<const grunt::client::TransferResume*>(packet)->offset;
			[[fallthrough]];
		case grunt::Opcode::CMD_XFER_ACCEPT:
			state_ = survey? State::SURVEY_TRANSFER : State::PATCH_TRANSFER;
			transfer_chunk();
			break;
		case grunt::Opcode::CMD_XFER_CANCEL:
			state_ = survey? State::SURVEY_RESULT : State::CLOSED;
			break;
		default:
			state_ = State::CLOSED;
			break;
	}
}

void LoginHandler::handle_transfer_abort() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	transfer_state_.abort = true;
}

void LoginHandler::transfer_chunk() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto remaining = transfer_state_.size - transfer_state_.offset;
	std::uint16_t read_size = grunt::server::TransferData::MAX_CHUNK_SIZE;

	if(read_size > remaining) {
		read_size = static_cast<std::uint16_t>(remaining);
	}

	grunt::server::TransferData response;
	response.size = read_size;

	if(state_ == State::SURVEY_TRANSFER) {
		auto* survey_mpq = patcher_.survey_data(challenge_.platform, challenge_.os).data();
		survey_mpq += transfer_state_.offset;
		std::copy(survey_mpq, survey_mpq + read_size, response.chunk.data());
	} else {
		transfer_state_.file.read(response.chunk.data(), read_size);
	}

	transfer_state_.offset += read_size;
	send_chunk(response);
}

void LoginHandler::on_chunk_complete() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(transfer_state_.abort) {
		return;
	}

	if(transfer_state_.offset != transfer_state_.size) {
		transfer_chunk();
	} else {
		switch(state_) {
			case State::SURVEY_TRANSFER:
				state_ = State::SURVEY_RESULT;
				break;
			case State::PATCH_TRANSFER:
				state_ = State::CLOSED;
				break;
		}
	}
}

} // ember