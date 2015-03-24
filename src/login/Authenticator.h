/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Protocol.h"
#include <srp6/Server.h>
#include <shared/misc/User.h>
#include <memory>
#include <string>

namespace ember {

class ReconnectAuthenticator {
	std::string rcon_user_;
	Botan::SecureVector<Botan::byte> rcon_chall_;
	srp6::SessionKey sess_key_;

public:
	ReconnectAuthenticator(std::string username, const std::string& session_key,
	                       const Botan::SecureVector<Botan::byte>& bytes);
	bool proof_check(const protocol::ClientReconnectProof* proof);
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
	srp6::Generator gen_ = srp6::Generator::GROUP::_256_BIT;
	srp6::SessionKey sess_key_;
	User user_;

public:
	LoginAuthenticator(User user);
	ChallengeResponse challenge_reply();
	ProofResult proof_check(protocol::ClientLoginProof* proof);
	std::string session_key();
};

} //ember