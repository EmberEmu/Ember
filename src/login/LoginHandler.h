/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Actions.h"
#include "AccountService.h"
#include "Authenticator.h"
#include "IntegrityData.h"
#include "GameVersion.h"
#include "RealmList.h"
#include "PINAuthenticator.h"
#include "grunt/Packets.h"
#include "grunt/Handler.h"
#include <logger/Logging.h>
#include <shared/database/daos/UserDAO.h>
#include <botan/bigint.h>
#include <botan/secmem.h>
#include <array>
#include <fstream>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>

namespace ember {

struct FileMeta;
class Patcher;
class Metrics;

struct TransferState {
	std::ifstream file;
	std::uint64_t offset;
	std::uint64_t size;
	bool abort;
};

class LoginHandler final {
	using CharacterCount = std::unordered_map<std::uint32_t, std::uint32_t>;

	using StateContainer = std::variant<
		std::unique_ptr<LoginAuthenticator>,
		std::unique_ptr<ReconnectAuthenticator>,
		CharacterCount
	>;

	enum class State {
		INITIAL_CHALLENGE, LOGIN_PROOF, RECONNECT_PROOF, REQUEST_REALMS,
		SURVEY_INITIATE, SURVEY_TRANSFER, SURVEY_RESULT, 
		PATCH_INITIATE, PATCH_TRANSFER, 
		FETCHING_USER_LOGIN, FETCHING_USER_RECONNECT, FETCHING_SESSION,
		FETCHING_CHARACTER_DATA,
		WRITING_SESSION, WRITING_SURVEY,
		CLOSED
	} state_ = State::INITIAL_CHALLENGE;

	Metrics& metrics_;
	log::Logger* logger_;
	const Patcher& patcher_;
	const RealmList& realm_list_;
	const dal::UserDAO& user_src_;
	std::optional<User> user_;
	Botan::BigInt server_proof_;
	const std::string source_;
	const AccountService& acct_svc_;
	const IntegrityData* exe_data_;
	StateContainer state_data_;
	std::array<std::uint8_t, CHECKSUM_SALT_LEN> checksum_salt_;
	PINAuthenticator::SaltBytes pin_salt_;
	std::uint32_t pin_grid_seed_;
	grunt::client::LoginChallenge challenge_;
	TransferState transfer_state_;
	const bool locale_enforce_;

	void initiate_login(const grunt::Packet& packet);
	void initiate_file_transfer(const FileMeta& meta);

	void handle_login_proof(const grunt::Packet& packet);
	void handle_reconnect_proof(const grunt::Packet& packet);
	void handle_survey_result(const grunt::Packet& packet);
	void handle_transfer_ack(const grunt::Packet& packet, bool survey);
	void handle_transfer_abort();

	void send_login_challenge(const FetchUserAction& action);
	void send_login_proof(grunt::Result result, bool survey = false);
	void send_reconnect_challenge(const FetchSessionKeyAction& action);
	void send_reconnect_proof(grunt::Result result);
	void send_realm_list(const grunt::Packet& packet);
	void build_login_challenge(grunt::server::LoginChallenge& packet);

	void on_character_data(const FetchCharacterCounts& action);
	void on_session_write(const RegisterSessionAction& action);
	void on_survey_write(const SaveSurveyAction& action);

	void transfer_chunk();
	void set_transfer_offset(const grunt::Packet& packet);

	bool validate_pin(const grunt::client::LoginProof& packet);
	bool validate_protocol_version(const grunt::client::LoginChallenge& challenge);

	bool validate_client_integrity(std::span<const std::uint8_t> client_hash,
								   const Botan::BigInt& client_salt, bool reconnect);

	bool validate_client_integrity(std::span<const std::uint8_t> client_hash,
	                               std::span<const std::uint8_t> client_salt, bool reconnect);

	void fetch_user(grunt::Opcode opcode, const utf8_string& username);
	void fetch_session_key(const FetchUserAction& action);

	void reject_client(const GameVersion& version);
	void patch_client(const grunt::client::LoginChallenge& version);

public:
	std::function<void(const std::shared_ptr<Action>& action)> execute_async;
	std::function<void(const grunt::Packet&)> send;
	std::function<void(const grunt::Packet&)> send_chunk;

	bool update_state(const Action& action);
	bool update_state(const grunt::Packet& packet);
	void on_chunk_complete();

	LoginHandler(const dal::UserDAO& users, const AccountService& acct_svc, const Patcher& patcher,
	             const IntegrityData* exe_data, log::Logger* logger, const RealmList& realm_list,
	             std::string source, Metrics& metrics, bool locale_enforce)
	             : user_src_(users), patcher_(patcher), logger_(logger), acct_svc_(acct_svc),
	               realm_list_(realm_list), source_(std::move(source)), metrics_(metrics),
	               exe_data_(exe_data), transfer_state_{}, locale_enforce_(locale_enforce) { }
};

} // ember