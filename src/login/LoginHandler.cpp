/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginHandler.h"
#include "AccountClient.h"
#include "ExecutablesChecksum.h"
#include "IntegrityData.h"
#include "LocaleMap.h"
#include "Patcher.h"
#include "RealmList.h"
#include "Survey.h"
#include "grunt/Packets.h"
#include <logger/Logger.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/game/GameVersion.h>
#include <shared/metrics/Metrics.h>
#include <shared/utility/EnumHelper.h>
#include <shared/utility/HashDefines.h>
#include <shared/utility/Utility.h>
#include <boost/container/small_vector.hpp>
#include <gsl/narrow>
#include <ranges>
#include <stdexcept>

namespace ember {

bool LoginHandler::update_state(const grunt::Packet& packet) try {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	const LoginState prev_state = state_;
	update_state(LoginState::closed);

	switch(prev_state) {
		case LoginState::challenge:
			handle_login_challenge(packet);
			break;
		case LoginState::spoof:
			handle_login_spoof(packet);
			break;
		case LoginState::proof:
			handle_login_proof(packet);
			break;
		case LoginState::reconnect_proof:
			handle_reconnect_proof(packet);
			break;
		case LoginState::request_realms:
			send_realm_list(packet);
			break;
		case LoginState::survey_initiate:
			handle_transfer_ack(packet, true);
			break;
		case LoginState::patch_initiate:
			handle_transfer_ack(packet, false);
			break;
			[[fallthrough]];
		case LoginState::survey_transfer:
		case LoginState::patch_transfer:
			handle_transfer_abort();
			break;
		case LoginState::survey_result:
			handle_survey_result(packet);
			break;
		case LoginState::closed:
			return false;
		default:
			LOG_DEBUG_ASYNC(logger_, "Received packet out of sync");
			return false;
	}

	return true;
} catch(const std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	update_state(LoginState::closed);
	return false;
}

bool LoginHandler::update_state(const Action& action) try {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	const LoginState prev_state = state_;
	update_state(LoginState::closed);

	switch(prev_state) {
		case LoginState::fetching_user_login:
			send_login_challenge(static_cast<const FetchUserAction&>(action));
			break;
		case LoginState::fetching_user_reconnect:
			fetch_session_key(static_cast<const FetchUserAction&>(action));
			break;
		case LoginState::fetching_session:
			send_reconnect_challenge(static_cast<const FetchSessionKeyAction&>(action));
			break;
		case LoginState::writing_session:
			on_session_write(static_cast<const RegisterSessionAction&>(action));
			break;
		case LoginState::request_realms:
			on_survey_write(static_cast<const SaveSurveyAction&>(action));
			break;
		case LoginState::fetching_character_data:
			on_character_data(static_cast<const FetchCharacterCounts&>(action));
			break;
		case LoginState::closed:
			return false;
		default:
			LOG_WARN_ASYNC(logger_, "Received action out of sync");
			return false;
	}

	return true;
} catch(const std::exception& e) {
	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	update_state(LoginState::closed);
	return false;
}

void LoginHandler::handle_login_challenge(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto& challenge = dynamic_cast<const grunt::client::LoginChallenge&>(packet);

	/*
	 * Older clients are likely to be using an older protocol version
	 * but they're close enough that patch transfers will still work
	 */
	if(!validate_protocol_version(challenge)) {
		LOG_DEBUG_ASYNC(logger_, "Unsupported protocol version {} ({})",
		                challenge.protocol_ver, identifier_);
	}

	if(challenge.game != grunt::Game::WoW) {
		LOG_DEBUG_ASYNC(logger_, "Bad game magic ({})", identifier_);
		update_state(LoginState::closed);
		return;
	}

	LOG_DEBUG_ASYNC(logger_, "Challenge: {}, {} ({})", challenge.username,
	                to_string(challenge.version), identifier_);

	const Patcher::PatchLevel level = patcher_.check_version(challenge.version);

	switch(level) {
		case Patcher::PatchLevel::ok:
			fetch_user(challenge.opcode, challenge.username);
			break;
		case Patcher::PatchLevel::too_new:
			reject_client(challenge.version);
			break;
		case Patcher::PatchLevel::too_old:
			patch_client(challenge);
			break;
	}

	challenge_ = challenge;
}

bool LoginHandler::validate_protocol_version(const grunt::client::LoginChallenge& challenge) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	const auto version = challenge.protocol_ver;

	if(challenge.opcode == grunt::Opcode::cmd_auth_logon_challenge
		&& version == grunt::client::LoginChallenge::challenge_version) {
		return true;
	}

	if(challenge.opcode == grunt::Opcode::cmd_auth_reconnect_challenge
		&& version == grunt::client::ReconnectChallenge::reconnect_challenge_version) {
		return true;
	}

	return false;
}

void LoginHandler::fetch_user(grunt::Opcode opcode, const utf8_string& username) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	switch(opcode) {
		case grunt::Opcode::cmd_auth_logon_challenge:
			update_state(LoginState::fetching_user_login);
			break;
		case grunt::Opcode::cmd_auth_reconnect_challenge:
			update_state(LoginState::fetching_user_reconnect);
			break;
		default:
			update_state(LoginState::closed);
			BOOST_ASSERT_MSG(false, "Impossible fetch_user condition");
	}

	auto action = std::make_unique<FetchUserAction>(username, user_src_);
	execute_async(std::move(action));
}

void LoginHandler::fetch_session_key(const FetchUserAction& action_res) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(!(user_ = action_res.get_result())) {
		LOG_DEBUG_ASYNC(logger_, "Account not found: {}", action_res.username());
		return;
	}

	update_state(LoginState::fetching_session);
	auto action = std::make_unique<FetchSessionKeyAction>(acct_svc_, user_->id());
	execute_async(std::move(action));
}

void LoginHandler::reject_client(const GameVersion& version) {
	LOG_DEBUG_ASYNC(logger_, "Rejecting client version {}", to_string(version));

	grunt::server::LoginChallenge response;
	response.result = grunt::Result::fail_version_update;
	send(response);
}

void LoginHandler::build_login_challenge(grunt::server::LoginChallenge& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	const auto& authenticator = std::get<LoginAuthenticator>(state_data_);
	const auto& values = authenticator.challenge_reply();
	packet.B = values.B;
	packet.g_len = gsl::narrow<std::uint8_t>(values.gen.generator().bytes());
	packet.g = gsl::narrow<std::uint8_t>(utility::to_u32bit(values.gen.generator()));
	packet.n_len = grunt::server::LoginChallenge::PRIME_LENGTH;
	packet.N = values.gen.prime();
	packet.s = values.salt;
	packet.two_factor_auth = false;

	if(user_ && user_->pin_method() != PINMethod::none) {
		packet.two_factor_auth = true;
		packet.pin_grid_seed = pin_grid_seed_ = PINAuthenticator::generate_seed();
		packet.pin_salt = pin_salt_ = PINAuthenticator::generate_salt();
	}

	Botan::AutoSeeded_RNG().randomize(checksum_salt_);
	packet.checksum_salt = checksum_salt_;
}

void LoginHandler::build_login_challenge_decoy(grunt::server::LoginChallenge& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto salt = Botan::AutoSeeded_RNG().random_array<32>();

	const std::string_view verifier {
		"0x399CF53C149F220F4AA88F7F2F6CA9CB6E4C44EA5240AC0F65601F392F32A16A"
	};

	state_data_.emplace<LoginAuthenticator>("dummy", verifier, salt);
	build_login_challenge(packet);
}

grunt::Result LoginHandler::process_fetch_user_action(const FetchUserAction& action) try {
	if((user_ = action.get_result())) {
		if(user_->verified() || !opts_.verified_email) {
			state_data_.emplace<LoginAuthenticator>(
				user_->username(), user_->verifier(), user_->salt()
			);

			return grunt::Result::success;
		} else {
			LOG_DEBUG_ASYNC(logger_, "Account not verified: {}", user_->username());
		}
	} else {
		LOG_DEBUG_ASYNC(logger_, "Account not found: {}", action.username());
	}

	return grunt::Result::fail_unknown_account;
} catch(const dal::exception& e) {
	metrics_.increment("login_internal_failure");
	LOG_ERROR_ASYNC(logger_, "DAL failure for {}: {}", action.username(), e.what());
	return grunt::Result::fail_db_busy;
} catch(const Botan::Exception& e) {
	metrics_.increment("login_internal_failure");
	LOG_ERROR_ASYNC(logger_, "Encoding failure for {}: {}", action.username(), e.what());
	return grunt::Result::fail_db_busy;
}

void LoginHandler::send_login_challenge(const FetchUserAction& action) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	grunt::server::LoginChallenge response;
	response.result = process_fetch_user_action(action);

	if(response.result == grunt::Result::success) {
		build_login_challenge(response);
		update_state(LoginState::proof);
	} else if(response.result == grunt::Result::fail_unknown_account) {
		build_login_challenge_decoy(response);
		update_state(LoginState::spoof);
		response.result = grunt::Result::success;
	}

	send(response);
}

void LoginHandler::send_reconnect_proof(grunt::Result result) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	LOG_DEBUG_ASYNC(logger_, "Reconnect result for {}: {}", user_->username(), grunt::to_string(result));

	if(result == grunt::Result::success) {
		metrics_.increment("login_success");
	} else {
		metrics_.increment("login_failure");
	}

	grunt::server::ReconnectProof response;
	response.result = result;
	send(response);
}

void LoginHandler::send_reconnect_challenge(const FetchSessionKeyAction& action) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	grunt::server::ReconnectChallenge response;
	response.result = grunt::Result::success;

	Botan::AutoSeeded_RNG().randomize(checksum_salt_);
	response.salt = checksum_salt_;

	const auto& [status, key] = action.get_result();

	if(status == rpc::Account::Status::ok) {
		update_state(LoginState::reconnect_proof);
		state_data_.emplace<ReconnectAuthenticator>(user_->username(), key, checksum_salt_);
	} else if(status == rpc::Account::Status::session_not_found) {
		metrics_.increment("login_failure");
		response.result = grunt::Result::fail_noaccess;
		LOG_DEBUG_ASYNC(logger_, "Reconnect failed, session not found for {}", user_->username());
	} else {
		metrics_.increment("login_internal_failure");
		response.result = grunt::Result::fail_db_busy;
		LOG_ERROR_ASYNC(logger_, "{} from peer during reconnect challenge",
		                utility::fb_status(status, rpc::Account::EnumNamesStatus()));
	}

	send(response);
}

bool LoginHandler::validate_pin(const grunt::client::LoginProof& packet) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	// no PIN was expected, nothing to validate
	if(user_->pin_method() == PINMethod::none) {
		return true;
	}

	// PIN auth is enabled for this user, make sure the packet has PIN data
	if(!packet.two_factor_auth) {
		return false;
	}

	PINAuthenticator pin_auth(pin_grid_seed_);
	bool result = false;

	if(user_->pin_method() == PINMethod::totp) {
		for(const auto interval : { 0, -1, 1 }) { // try time intervals -1 to +1
			const auto pin = PINAuthenticator::generate_totp_pin(user_->totp_token(), interval);

			if(pin_auth.validate_pin(pin_salt_, packet.pin_salt, packet.pin_hash, pin)) {
				result = true;
				break;
			}
		}
	} else if(user_->pin_method() == PINMethod::static_fixed) {
		result = pin_auth.validate_pin(pin_salt_, packet.pin_salt, packet.pin_hash, user_->pin());
	} else {
		LOG_ERROR_ASYNC(logger_, "Unknown TOTP method, {}", std::to_underlying(user_->pin_method()));
	}

	LOG_DEBUG_ASYNC(logger_, "PIN authentication for {} {}", user_->username(), result? "OK" : "failed");
	return result;
}

bool LoginHandler::validate_client_integrity(std::span<const std::uint8_t> hash,
                                             const Botan::BigInt& salt,
                                             bool reconnect) const {
	constexpr auto expected_len = 32u;

	boost::container::small_vector<std::uint8_t, expected_len> bytes(
		salt.bytes(), boost::container::default_init
	);

	salt.serialize_to(bytes);
	std::ranges::reverse(bytes);

	return validate_client_integrity(hash, bytes, reconnect);
}

bool LoginHandler::validate_client_integrity(std::span<const std::uint8_t> client_hash,
                                             std::span<const uint8_t> salt,
                                             const bool reconnect) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	// client doesn't bother to checksum the binaries on reconnect, it just hashes the salt (=])
	std::array<std::uint8_t, hash_sizes::sha160> checksum{}; // all-zero hash

	if(!reconnect) {
		const auto data = bin_data_.lookup(challenge_.version, challenge_.platform, challenge_.os);

		// ensure we have binaries for the platform/version the client is using
		if(!data) {
			return false;
		}

		checksum = client_integrity::checksum(checksum_salt_, *data);
	}

	const auto hash = client_integrity::finalise(checksum, salt);
	return std::ranges::equal(hash, client_hash);
}

void LoginHandler::handle_login_proof(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto& proofs = dynamic_cast<const grunt::client::LoginProof&>(packet);

	if(opts_.integrity_check && !validate_client_integrity(proofs.client_checksum, proofs.A, false)) {
		send_login_proof(grunt::Result::fail_version_invalid);
		return;
	}

	if(!validate_pin(proofs)) {
		send_login_proof(grunt::Result::fail_incorrect_password);
		return;
	}

	const auto& authenticator = std::get<LoginAuthenticator>(state_data_);
	const auto key = authenticator.session_key(proofs.A);
	const auto expected = authenticator.expected_proof(key, proofs.A);
	auto result = grunt::Result::fail_incorrect_password;
	
	if(proofs.M1 == expected) {
		if(user_->banned()) {
			result = grunt::Result::fail_banned;
		} else if(user_->suspended()) {
			result = grunt::Result::fail_suspended;
		} else if(!user_->subscriber()) {
			result = grunt::Result::fail_no_time;
		/*} else if(parental_controls) {
			result = grunt::Result::FAIL_PARENTAL_CONTROLS;*/
		} else {
			result = grunt::Result::success;
		}
	}

	if(result == grunt::Result::success) {
		update_state(LoginState::writing_session);
		server_proof_ = authenticator.server_proof(key, proofs.A, proofs.M1);

		auto action = std::make_unique<RegisterSessionAction>(
			acct_svc_, user_->id(),
			authenticator.session_key(proofs.A)
		);

		execute_async(std::move(action));
	} else {
		send_login_proof(result);
	}
}

void LoginHandler::handle_login_spoof(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto& proof_packet = dynamic_cast<const grunt::client::LoginProof&>(packet);

	if(opts_.integrity_check && !validate_client_integrity(proof_packet.client_checksum, proof_packet.A, false)) {
		send_login_proof(grunt::Result::fail_version_invalid);
		return;
	}

	send_login_proof(grunt::Result::fail_incorrect_password);
}

void LoginHandler::send_login_proof(grunt::Result result, bool survey) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	grunt::server::LoginProof response;
	response.result = result;

	if(result == grunt::Result::success) {
		metrics_.increment("login_success");
		response.M2 = server_proof_;
		response.survey_id = survey? survey_.id() : 0;
	} else {
		metrics_.increment("login_failure");
	}

	if(user_) {
		LOG_DEBUG_ASYNC(logger_, "Login result for {}: {}", user_->username(), grunt::to_string(result));
	}

	send(response);
}

void LoginHandler::on_character_data(const FetchCharacterCounts& action) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	try {
		state_data_ = action.get_result();
	} catch(const dal::exception& e) { // not a fatal exception, we'll keep going without the data
		state_data_ = CharacterCount();
		metrics_.increment("login_internal_failure");
		LOG_ERROR_ASYNC(logger_, "DAL failure for {}: {}", user_->username(), e.what());
	}

	update_state(LoginState::request_realms);

	if(action.reconnect()) {
		send_reconnect_proof(grunt::Result::success);
		return;
	}
	
	if(user_->survey_request() && survey_.id()
	   && survey_.data(challenge_.platform, challenge_.os)) {
		update_state(LoginState::survey_initiate);
	}

	send_login_proof(grunt::Result::success, state_ == LoginState::survey_initiate);

	if(state_ == LoginState::survey_initiate) {
		LOG_DEBUG_ASYNC(logger_, "Initiating survey transfer...");
		auto meta = survey_.meta(challenge_.platform, challenge_.os);
		assert(meta);
		initiate_file_transfer(*meta);
	}
}

void LoginHandler::on_session_write(const RegisterSessionAction& action) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto result = action.get_result();
	grunt::Result response = grunt::Result::success;

	if(result == rpc::Account::Status::ok) {
		update_state(LoginState::fetching_character_data);
	} else if(result == rpc::Account::Status::already_logged_in) {
		response = grunt::Result::fail_already_online;
	} else {
		metrics_.increment("login_internal_failure");
		response = grunt::Result::fail_db_busy;
		LOG_ERROR_ASYNC(logger_, "{} from peer during login",
		                utility::fb_status(result, rpc::Account::EnumNamesStatus()));
	}

	// defer sending the response until we've fetched the character data
	if(result == rpc::Account::Status::ok) {
		execute_async(std::make_unique<FetchCharacterCounts>(user_->id(), user_src_));
	} else {
		send_login_proof(response);
	}
}

void LoginHandler::handle_reconnect_proof(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto& reconn_proof = dynamic_cast<const grunt::client::ReconnectProof&>(packet);

	if(opts_.integrity_check && !validate_client_integrity(reconn_proof.client_checksum, reconn_proof.salt, true)) {
		send_reconnect_proof(grunt::Result::fail_version_invalid);
		return;
	}

	const auto& authenticator = std::get<ReconnectAuthenticator>(state_data_);

	if(authenticator.proof_check(reconn_proof.salt, reconn_proof.proof)) {
		update_state(LoginState::fetching_character_data);
		execute_async(std::make_unique<FetchCharacterCounts>(user_->id(), user_src_, true));
	} else {
		send_reconnect_proof(grunt::Result::fail_incorrect_password);
	}
}

void LoginHandler::send_realm_list(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	// todo - move these checks (inc. dynamic_casts) out of the handler functions
	if(packet.opcode != grunt::Opcode::cmd_realm_list) {
		throw std::runtime_error("Expected CMD_REALM_LIST");
	}

	// look the client's locale up for sending the correct realm category
	auto it = locale_map.find(challenge_.locale);

	if(it == locale_map.end()) {
		return;
	}

	const auto& [_, region] = *it;
	const std::shared_ptr<const RealmMap> realms = realm_list_.realms();
	const auto& char_count = std::get<CharacterCount>(state_data_);
	grunt::server::RealmList response;

	for(const auto& realm : *realms | std::views::values) {
		if(!opts_.locales || realm.region == region) {
			if(auto count = char_count.find(realm.id); count != char_count.end()) {
				response.realms.emplace_back(realm, count->second);
			} else {
				response.realms.emplace_back(realm, 0);
			}
		}
	}

	update_state(LoginState::request_realms);
	send(response);
}

void LoginHandler::patch_client(const grunt::client::LoginChallenge& challenge) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto meta = patcher_.find_patch(challenge.version, challenge.locale,
	                                challenge.platform, challenge.os);

	if(!meta) {
		reject_client(challenge.version);
		return;
	}

	grunt::server::LoginChallenge response;
	response.result = grunt::Result::fail_version_update;
	send(response);

	auto& fmeta = meta->file_meta;

	LOG_DEBUG_ASYNC(logger_, "Initiating patch transfer, {}", fmeta.name);
	std::ifstream patch(fmeta.path + fmeta.name, std::ifstream::binary);

	if(!patch) {
		LOG_ERROR_ASYNC(logger_, "Could not open patch, {}", fmeta.name);
		return;
	}

	transfer_state_.file = std::move(patch);
	
	if(meta->mpq) {
		fmeta.name = "Patch";
	}

	metrics_.increment("patches_sent");
	update_state(LoginState::patch_initiate);
	initiate_file_transfer(fmeta);
}

void LoginHandler::initiate_file_transfer(const FileMeta& meta) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	
	transfer_state_.size = meta.size;

	grunt::server::TransferInitiate response;
	response.filename = meta.name;
	response.filesize = meta.size;
	response.md5 = meta.md5;
	send(response);
}

void LoginHandler::handle_survey_result(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto& survey = dynamic_cast<const grunt::client::SurveyResult&>(packet);

	// allow the client to request the realmlist without waiting on the survey write callback
	update_state(LoginState::request_realms);

	if(survey.survey_id != survey_.id()) {
		LOG_DEBUG_ASYNC(logger_, "Received an invalid survey ID from {}", user_->username());
		return;
	}

	/*
	 * Errors can be caused by the client having already sent data for the
	 * active survey ID or by the compressed data length being too large
	 * for the client to send (hardcoded to 1000 bytes)
	 */
	if(survey.error) {
		return;
	}

	auto action = std::make_unique<SaveSurveyAction>(
		user_->id(), user_src_,
		survey.survey_id, survey.data
	);

	metrics_.increment("surveys_received");
	execute_async(std::move(action));
}

void LoginHandler::on_survey_write(const SaveSurveyAction& action) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(action.error()) {
		LOG_ERROR_ASYNC(logger_, "DAL failure for {}, {}", user_->username(), action.exception().what());
	}
}

void LoginHandler::set_transfer_offset(const grunt::Packet& packet) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto& resume = dynamic_cast<const grunt::client::TransferResume&>(packet);
	transfer_state_.offset = resume.offset;
}

void LoginHandler::handle_transfer_ack(const grunt::Packet& packet, bool survey) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	switch(packet.opcode) {
		case grunt::Opcode::cmd_xfer_resume:
			set_transfer_offset(packet);
			[[fallthrough]];
		case grunt::Opcode::cmd_xfer_accept:
			update_state(survey? LoginState::survey_transfer : LoginState::patch_transfer);
			transfer_chunk();
			break;
		case grunt::Opcode::cmd_xfer_cancel:
			update_state(survey? LoginState::survey_result : LoginState::closed);
			break;
		default:
			update_state(LoginState::closed);
			break;
	}
}

void LoginHandler::handle_transfer_abort() {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	transfer_state_.abort = true;
}

void LoginHandler::transfer_chunk() {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	auto remaining = transfer_state_.size - transfer_state_.offset;
	std::uint16_t read_size = grunt::server::TransferData::MAX_CHUNK_SIZE;

	if(read_size > remaining) {
		read_size = gsl::narrow<std::uint16_t>(remaining);
	}

	grunt::server::TransferData response;
	response.size = read_size;

	if(state_ == LoginState::survey_transfer) {
		auto survey_mpq = survey_.data(challenge_.platform, challenge_.os)->begin();
		survey_mpq += gsl::narrow<int>(transfer_state_.offset);
		std::copy(survey_mpq, survey_mpq + read_size, response.chunk.data());
	} else {
		transfer_state_.file.read(reinterpret_cast<char*>(response.chunk.data()), read_size);

		if(!transfer_state_.file) {
			LOG_ERROR_ASYNC(logger_, "Patch reading failed during transfer");
			return;
		}
	}

	transfer_state_.offset += read_size;

	send_notify(response, [this]() {
		on_chunk_complete();
	});
}

void LoginHandler::on_chunk_complete() {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(transfer_state_.abort) {
		return;
	}

	// transfer complete?
	if(transfer_state_.offset == transfer_state_.size) {
		switch(state_) {
			case LoginState::survey_transfer:
				update_state(LoginState::survey_result);
				break;
			case LoginState::patch_transfer:
				update_state(LoginState::closed);
				break;
			default:
				break;
		}
	} else {
		transfer_chunk();
	}
}

inline void LoginHandler::update_state(const LoginState& state) {
	state_ = state;
}

} // ember
