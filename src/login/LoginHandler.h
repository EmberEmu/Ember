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
#include "ExecutablesChecksum.h"
#include "GameVersion.h"
#include "RealmList.h"
#include "PINAuthenticator.h"
#include "grunt/Packets.h"
#include "grunt/Handler.h"
#include <logger/Logging.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/metrics/Metrics.h>
#include <botan/bigint.h>
#include <botan/secmem.h>
#include <boost/optional.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ember {

class Patcher;
class NetworkSession;
class Metrics;

class LoginHandler {
	const static int HASH_LENGTH = 20;

	enum class State {
		INITIAL_CHALLENGE, LOGIN_PROOF, RECONNECT_PROOF, REQUEST_REALMS,
		FETCHING_USER_LOGIN, FETCHING_USER_RECONNECT, FETCHING_SESSION,
		WRITING_SESSION, FETCHING_CHARACTER_DATA, CLOSED
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
	const ExecutableChecksum* exe_checksum_;
	PINAuthenticator pin_auth_;
	std::unique_ptr<LoginAuthenticator> login_auth_;
	std::unique_ptr<ReconnectAuthenticator> reconn_auth_;
	std::unordered_map<std::uint32_t, std::uint32_t> char_count_;
	Botan::SecureVector<Botan::byte> checksum_salt_;

	void send_realm_list(const grunt::Packet* packet);
	void initiate_login(const grunt::Packet* packet);
	void check_login_proof(const grunt::Packet* packet);
	void check_reconnect_proof(const grunt::Packet* packet);
	void send_reconnect_proof(grunt::ResultCode result);
	void send_login_proof(grunt::ResultCode result);
	void build_login_challenge(grunt::server::LoginChallenge& packet);
	void send_login_challenge(FetchUserAction* action);
	void send_reconnect_challenge(FetchSessionKeyAction* action);
	void on_character_data(FetchCharacterCounts* action);
	void on_session_write(RegisterSessionAction* action);
	bool validate_pin(const grunt::client::LoginProof* packet);
	
	bool validate_client_integrity(const std::array<std::uint8_t, 20>& client_hash,
								   const Botan::BigInt& client_salt, bool reconnect);

	bool validate_client_integrity(const std::array<std::uint8_t, 20>& client_hash,
	                               const std::uint8_t* client_salt, std::size_t len,
	                               bool reconnect);

	void fetch_user(grunt::Opcode opcode, const std::string& username);
	void fetch_session_key(FetchUserAction* action);

	void reject_client(const GameVersion& version);
	void patch_client(const GameVersion& version);

public:
	std::function<void(std::shared_ptr<Action> action)> execute_async;
	std::function<void(const grunt::Packet&)> send;

	bool update_state(std::shared_ptr<Action> action);
	bool update_state(const grunt::Packet* packet);

	LoginHandler(const dal::UserDAO& users, const AccountService& acct_svc, const Patcher& patcher,
	             const ExecutableChecksum* exe_checksum, log::Logger* logger,
	             const RealmList& realm_list, std::string source, Metrics& metrics)
	             : user_src_(users), patcher_(patcher), logger_(logger), acct_svc_(acct_svc),
	               realm_list_(realm_list), source_(std::move(source)), metrics_(metrics),
	               pin_auth_(logger), exe_checksum_(exe_checksum) { }
};

} // ember