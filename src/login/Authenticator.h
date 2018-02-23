/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "grunt/Packets.h"
#include <srp6/Server.h>
#include <shared/database/objects/User.h>
#include <memory>
#include <string>

namespace ember {

class ReconnectAuthenticator {
	std::string rcon_user_;
	Botan::secure_vector<Botan::byte> salt_;
	srp6::SessionKey sess_key_;

public:
	ReconnectAuthenticator(std::string username, const Botan::BigInt& session_key,
	                       const Botan::secure_vector<Botan::byte>& salt);
	bool proof_check(const grunt::client::ReconnectProof& proof);
	std::string username() { return rcon_user_; }
};

class LoginAuthenticator {
	struct ChallengeResponse {
		Botan::BigInt B;
		Botan::BigInt salt;
		srp6::Generator gen;
	};

	struct ProofResult {
		bool match;
		Botan::BigInt server_proof;
	};

	std::unique_ptr<srp6::Server> srp_;
	srp6::Generator gen_ { srp6::Generator::Group::_256_BIT };
	srp6::SessionKey sess_key_;
	User user_;

public:
	explicit LoginAuthenticator(User user);
	ChallengeResponse challenge_reply();
	ProofResult proof_check(const grunt::client::LoginProof& proof);
	srp6::SessionKey session_key();
};

} //ember