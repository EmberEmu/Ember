/*
 * Copyright (c) 2015, 2016 Ember
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
#include <boost/optional.hpp>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

class LoginHandler {
	enum class State {
		INITIAL_CHALLENGE, LOGIN_PROOF, RECONNECT_PROOF, REQUEST_REALMS,
		SURVEY_INITIATE, SURVEY_TRANSFER, SURVEY_RESULT, 
		PATCH_INITIATE, PATCH_TRANSFER, 
		FETCHING_USER_LOGIN, FETCHING_USER_RECONNECT, FETCHING_SESSION,
		FETCHING_CHARACTER_DATA,
		WRITING_SESSION, WRITING_SURVEY,
		CLOSED
	};

	State state_ = State::INITIAL_CHALLENGE;
	Metrics& metrics_;
	log::Logger* logger_;
	const Patcher& patcher_;
	const RealmList& realm_list_;
	const dal::UserDAO& user_src_;
	boost::optional<User> user_;
	Botan::BigInt server_proof_;
	const std::string source_;
	const AccountService& acct_svc_;
	const IntegrityData* exe_data_;
	PINAuthenticator pin_auth_;
	std::unique_ptr<LoginAuthenticator> login_auth_;
	std::unique_ptr<ReconnectAuthenticator> reconn_auth_;
	std::unordered_map<std::uint32_t, std::uint32_t> char_count_;
	Botan::secure_vector<Botan::byte> checksum_salt_;
	grunt::client::LoginChallenge challenge_;
	TransferState transfer_state_;
	const bool locale_enforce_;

	void initiate_login(const grunt::Packet* packet);
	void initiate_file_transfer(const FileMeta& meta);

	void handle_login_proof(const grunt::Packet* packet);
	void handle_reconnect_proof(const grunt::Packet* packet);
	void handle_survey_result(const grunt::Packet* packet);
	void handle_transfer_ack(const grunt::Packet* packet, bool survey);
	void handle_transfer_abort();

	void send_login_challenge(FetchUserAction* action);
	void send_login_proof(grunt::Result result, bool survey = false);
	void send_reconnect_challenge(FetchSessionKeyAction* action);
	void send_reconnect_proof(grunt::Result result);
	void send_realm_list(const grunt::Packet* packet);
	void build_login_challenge(grunt::server::LoginChallenge& packet);

	void on_character_data(FetchCharacterCounts* action);
	void on_session_write(RegisterSessionAction* action);
	void on_survey_write(SaveSurveyAction* action);

	void transfer_chunk();
	void set_transfer_offset(const grunt::Packet* packet);

	bool validate_pin(const grunt::client::LoginProof* packet);
	bool validate_protocol_version(const grunt::client::LoginChallenge* challenge);

	bool validate_client_integrity(const std::array<std::uint8_t, 20>& client_hash,
								   const Botan::BigInt& client_salt, bool reconnect);

	bool validate_client_integrity(const std::array<std::uint8_t, 20>& client_hash,
	                               const std::uint8_t* client_salt, std::size_t len,
	                               bool reconnect);

	void fetch_user(grunt::Opcode opcode, const std::string& username);
	void fetch_session_key(FetchUserAction* action);

	void reject_client(const GameVersion& version);
	void patch_client(const grunt::client::LoginChallenge* version);

public:
	std::function<void(std::shared_ptr<Action> action)> execute_async;
	std::function<void(const grunt::Packet&)> send;
	std::function<void(const grunt::Packet&)> send_chunk;

	bool update_state(std::shared_ptr<Action> action);
	bool update_state(const grunt::Packet* packet);
	void on_chunk_complete();

	LoginHandler(const dal::UserDAO& users, const AccountService& acct_svc, const Patcher& patcher,
	             const IntegrityData* exe_data, log::Logger* logger, const RealmList& realm_list,
	             std::string source, Metrics& metrics, bool locale_enforce)
	             : user_src_(users), patcher_(patcher), logger_(logger), acct_svc_(acct_svc),
	               realm_list_(realm_list), source_(std::move(source)), metrics_(metrics),
	               pin_auth_(logger), exe_data_(exe_data), transfer_state_{},
				   locale_enforce_(locale_enforce) { }
};

} // ember