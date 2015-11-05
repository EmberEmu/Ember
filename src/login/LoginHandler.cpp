/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "grunt/Packets.h"
#include "LoginHandler.h"
#include "Patcher.h"
#include <boost/range/adaptor/map.hpp>
#include <stdexcept>
#include <utility>

namespace ember {

bool LoginHandler::update_state(const grunt::Packet* packet) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(state_) {
		case State::INITIAL_CHALLENGE:
			process_challenge(packet);
			break;
		case State::LOGIN_PROOF:
			check_login_proof(packet);
			break;
		case State::RECONNECT_PROOF:
			send_reconnect_proof(packet);
			break;
		case State::REQUEST_REALMS:
			send_realm_list(packet);
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

	switch(state_) {
		case State::FETCHING_USER:
			send_login_challenge(static_cast<FetchUserAction*>(action.get()));
			break;
		case State::FETCHING_SESSION:
			send_reconnect_challenge(static_cast<FetchSessionKeyAction*>(action.get()));
			break;
		case State::WRITING_SESSION:
			send_login_success(static_cast<StoreSessionAction*>(action.get()));
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

void LoginHandler::process_challenge(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto challenge = dynamic_cast<const grunt::client::LoginChallenge*>(packet);

	if(!challenge) {
		throw std::runtime_error("Expected CMSG_*_CHALLENGE");
	}

	if(challenge->magic != grunt::client::LoginChallenge::WoW) {
		throw std::runtime_error("Invalid game magic!");
	}

	LOG_DEBUG(logger_) << "Challenge: " << challenge->username << ", "
	                   << challenge->version << ", " << source_ << LOG_ASYNC;

	Patcher::PatchLevel patch_level = patcher_.check_version(challenge->version);

	switch(patch_level) {
		case Patcher::PatchLevel::OK:
			accept_client(challenge->opcode, challenge->username);
			break;
		case Patcher::PatchLevel::TOO_NEW:
		case Patcher::PatchLevel::TOO_OLD:
			reject_client(challenge->version);
			break;
		case Patcher::PatchLevel::PATCH_AVAILABLE:
			patch_client(challenge->version);
			break;
	}
}

void LoginHandler::patch_client(const GameVersion& version) {
	// don't know yet
	//stream << std::uint8_t(0) << std::uint8_t(0) << protocol::RESULT::FAIL_VERSION_UPDATE;
	//write(packet);
}

void LoginHandler::accept_client(grunt::client::Opcode opcode, const std::string& username) {
	std::shared_ptr<Action> action;

	switch(opcode) {
		case grunt::client::Opcode::CMSG_LOGIN_CHALLENGE:
			action = std::make_shared<FetchUserAction>(username, user_src_);
			state_ = State::FETCHING_USER;
			break;
		case grunt::client::Opcode::CMSG_RECONNECT_CHALLENGE:
			action = std::make_shared<FetchSessionKeyAction>(username, user_src_);
			state_ = State::FETCHING_SESSION;
			break;
		default:
			BOOST_ASSERT_MSG(false, "Impossible accept_client condition");
	}

	execute_async(action);
}

void LoginHandler::reject_client(const GameVersion& version) {
	LOG_DEBUG(logger_) << "Rejecting client version " << version << LOG_ASYNC;
	state_ = State::CLOSED;

	auto response = std::make_unique<grunt::server::LoginChallenge>();
	response->opcode = grunt::server::Opcode::SMSG_LOGIN_CHALLENGE;
	response->result = grunt::ResultCode::FAIL_VERSION_INVALID;
	send(std::move(response));
}

void LoginHandler::build_login_challenge(grunt::server::LoginChallenge* packet) {	
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto values = login_auth_->challenge_reply();
	packet->B = values.B;
	packet->g_len = static_cast<std::uint8_t>(values.gen.generator().bytes());
	packet->g = static_cast<std::uint8_t>(values.gen.generator().to_u32bit());
	packet->n_len = grunt::server::LoginChallenge::PRIME_LENGTH;
	packet->N = values.gen.prime();
	packet->s = values.salt;
	auto rand = Botan::AutoSeeded_RNG().random_vec(16);;
	std::copy(rand.begin(), rand.end(), packet->unk3.data());
}

void LoginHandler::send_login_challenge(FetchUserAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	state_ = State::CLOSED;
	
	auto response = std::make_unique<grunt::server::LoginChallenge>();
	response->opcode = grunt::server::Opcode::SMSG_LOGIN_CHALLENGE;
	response->result = grunt::ResultCode::SUCCESS;

	try {
		if((user_ = action->get_result())) {
			login_auth_ = std::make_unique<LoginAuthenticator>(*user_);
			build_login_challenge(response.get());
			state_ = State::LOGIN_PROOF;
		} else {
			// leaks information on whether the account exists (could send challenge anyway?)
			response->result = grunt::ResultCode::FAIL_UNKNOWN_ACCOUNT;
			metrics_.increment("login_failure");
			LOG_DEBUG(logger_) << "Account not found: " << action->username() << LOG_ASYNC;
		}
	} catch(dal::exception& e) {
		response->result = grunt::ResultCode::FAIL_DB_BUSY;
		metrics_.increment("login_internal_failure");
		LOG_ERROR(logger_) << "DAL failure for " << action->username()
		                   << " " << e.what() << LOG_ASYNC;
	} catch(Botan::Exception& e) {
		response->result = grunt::ResultCode::FAIL_DB_BUSY;
		metrics_.increment("login_internal_failure");
		LOG_ERROR(logger_) << "Encoding failure for " << action->username()
		                   << " " << e.what() << LOG_ASYNC;
	}
	
	send(std::move(response));
}

void LoginHandler::send_reconnect_challenge(FetchSessionKeyAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	state_ = State::CLOSED;

	auto response = std::make_unique<grunt::server::ReconnectChallenge>();
	response->opcode = grunt::server::Opcode::SMSG_RECONNECT_CHALLENGE;
	response->result = grunt::ResultCode::SUCCESS;
	response->rand = Botan::AutoSeeded_RNG().random_vec(16);

	try {
		boost::optional<std::string> key = action->get_result();

		if(key) {
			state_ = State::RECONNECT_PROOF;
			reconn_auth_ = std::make_unique<ReconnectAuthenticator>(action->username(), *key, response->rand);
		} else {
			response->result = grunt::ResultCode::FAIL_NOACCESS;
		}
	} catch(dal::exception& e) {
		response->result = grunt::ResultCode::FAIL_DB_BUSY;
		metrics_.increment("login_internal_failure");
		LOG_ERROR(logger_) << "Retrieving key for " << action->username()
		                   << ": " << e.what() << LOG_ASYNC;
	}

	send(std::move(response));
}

void LoginHandler::check_login_proof(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto proof_packet = dynamic_cast<const grunt::client::LoginProof*>(packet);

	if(!proof_packet) {
		throw std::runtime_error("Expected CMSG_LOGIN_PROOF");
	}

	auto proof = login_auth_->proof_check(proof_packet);
	auto result = grunt::ResultCode::FAIL_INCORRECT_PASSWORD;

	if(proof.match) {
		if(user_->banned()) {
			result = grunt::ResultCode::FAIL_BANNED;
		} else if(user_->suspended()) {
			result = grunt::ResultCode::FAIL_SUSPENDED;
			/*} else if(time) {
				res = grunt::ResultCode::FAIL_NO_TIME;
				} else if(parental_controls) {
				res = grunt::ResultCode::FAIL_PARENTAL_CONTROLS;*/
		} else {
			result = grunt::ResultCode::SUCCESS;
		}
	}

	if(result == grunt::ResultCode::SUCCESS) {
		state_ = State::WRITING_SESSION;
		server_proof_ = proof.server_proof;
		auto action = std::make_shared<StoreSessionAction>(*user_, source_, login_auth_->session_key(), user_src_);
		execute_async(action);
	} else {
		state_ = State::CLOSED;
		send_login_failure(result);
	}
}

void LoginHandler::send_login_failure(grunt::ResultCode result) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	metrics_.increment("login_failure");

	auto response = std::make_unique<grunt::server::LoginProof>();
	response->opcode = grunt::server::Opcode::SMSG_LOGIN_PROOF;
	response->result = result;

	send(std::move(response));
}

void LoginHandler::send_login_success(StoreSessionAction* action) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	metrics_.increment("login_success");

	auto response = std::make_unique<grunt::server::LoginProof>();
	response->opcode = grunt::server::Opcode::SMSG_LOGIN_PROOF;
	response->result = grunt::ResultCode::SUCCESS;
	response->M2 = server_proof_;
	response->account_flags = 0;

	state_ = State::REQUEST_REALMS;
	send(std::move(response));
}

void LoginHandler::send_reconnect_proof(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto proof = dynamic_cast<const grunt::client::ReconnectProof*>(packet);

	if(!proof) {
		throw std::runtime_error("Expected CMSG_RECONNECT_PROOF");
	}

	if(reconn_auth_->proof_check(proof)) {
		LOG_DEBUG(logger_) << "Successfully reconnected " << reconn_auth_->username() << LOG_ASYNC;
	} else {
		LOG_DEBUG(logger_) << "Failed to reconnect " << reconn_auth_->username() << LOG_ASYNC;
		return;
	}

	auto response = std::make_unique<grunt::server::ReconnectProof>();
	response->opcode = grunt::server::Opcode::SMSG_RECONNECT_PROOF;
	response->result = grunt::ResultCode::SUCCESS;

	state_ = State::REQUEST_REALMS;
	send(std::move(response));
}

void LoginHandler::send_realm_list(const grunt::Packet* packet) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!dynamic_cast<const grunt::client::RequestRealmList*>(packet)) {
		throw std::runtime_error("Expected CMSG_REQUEST_REALM_LIST");
	}

	auto response = std::make_unique<grunt::server::RealmList>();
	response->opcode = grunt::server::Opcode::SMSG_REQUEST_REALM_LIST;
	
	std::shared_ptr<const RealmMap> realms = realm_list_.realms();

	for(auto& realm : *realms | boost::adaptors::map_values) {
		response->realms.push_back({ realm, 0 });
	}

	send(std::move(response));
}

} // ember