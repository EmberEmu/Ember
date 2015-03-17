/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "Protocol.h"
#include <srp6/Server.h>
#include <shared/misc/User.h>
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace ember {

namespace dal { class UserDAO; }

class Authenticator {
	dal::UserDAO& users_;
	boost::optional<User> user_;

	const std::vector<GameVersion>& versions_;
	std::unique_ptr<srp6::Server> auth_;
	srp6::Generator gen_ = srp6::Generator::GROUP::_256_BIT;
	srp6::SessionKey sess_key_;
	Botan::SecureVector<Botan::byte> rcon_chall_;
	std::string rcon_user_;

	struct ChallengeResponse {
		Botan::BigInt B;
		Botan::BigInt salt;
		srp6::Generator gen;
	};

public:
	typedef std::pair<protocol::ResultCodes, Botan::BigInt> LoginResult;
	enum class PatchState { OK, TOO_OLD, TOO_NEW };
	enum class AccountStatus { OK, NOT_FOUND, DAL_ERROR };

	Authenticator(dal::UserDAO& users, const std::vector<GameVersion>& versions)
	              : versions_(versions), users_(users) { }

	//VS2013 bug workaround, again
	Authenticator(Authenticator&& rhs) : versions_(rhs.versions_), users_(rhs.users_) {
		auth_ = std::move(rhs.auth_);
	}

	Authenticator(const Authenticator&) = delete;
	Authenticator& operator=(const Authenticator&) = delete;

	ChallengeResponse challenge_reply();
	LoginResult proof_check(protocol::ClientLoginProof* proof);

	void set_logged_in(const std::string& ip);
	PatchState verify_client_version(const GameVersion& version);
	AccountStatus check_account(const std::string& username);

	bool begin_reconnect(const std::string& username);
	bool reconnect_proof_check(protocol::ClientReconnectProof* proof);
	void set_reconnect_challenge(const Botan::SecureVector<Botan::byte>& bytes);
	void set_session_key();
};

} //ember