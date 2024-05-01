/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authenticator.h"
#include "grunt/client/ReconnectProof.h"
#include <logger/Logging.h>
#include <srp6/Server.h>
#include <srp6/Client.h>
#include <shared/database/daos/UserDAO.h>
#include <botan/hash.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <utility>

namespace ember {

LoginAuthenticator::LoginAuthenticator(User user)
	: user_(std::move(user)),
	  srp_(gen_, Botan::BigInt(user_.verifier())) {}

auto LoginAuthenticator::challenge_reply() -> ChallengeResponse {
	return {srp_.public_ephemeral(), Botan::BigInt(user_.salt()), gen_};
}

Botan::BigInt LoginAuthenticator::server_proof(const srp6::SessionKey& key,
                                               const Botan::BigInt& M1) {
	return srp_.generate_proof(key, M1);
}

Botan::BigInt LoginAuthenticator::expected_proof(const srp6::SessionKey& key,
                                                 const Botan::BigInt& A) {
	// Usernames aren't required to be uppercase in the DB but the client requires it for calculations
	utf8_string user_upper(user_.username());
	std::transform(user_upper.begin(), user_upper.end(), user_upper.begin(), ::toupper);

	Botan::BigInt B = srp_.public_ephemeral();
	return srp6::generate_client_proof(user_upper, key, gen_.prime(), gen_.generator(),
	                                   A, B, user_.salt());
}

srp6::SessionKey LoginAuthenticator::session_key(const Botan::BigInt& A) {
	return srp_.session_key(A);
}

ReconnectAuthenticator::ReconnectAuthenticator(utf8_string username, const Botan::BigInt& session_key,
                                               const std::array<std::uint8_t, CHECKSUM_SALT_LEN>& salt)
                                               : rcon_user_(std::move(username)), salt_(salt) {
	// Usernames aren't required to be uppercase in the DB but the client requires it for calculations
	std::transform(rcon_user_.begin(), rcon_user_.end(), rcon_user_.begin(), ::toupper);
	session_key.binary_encode(sess_key_.t.data(), sess_key_.t.size());
}

bool ReconnectAuthenticator::proof_check(const grunt::client::ReconnectProof& packet) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	hasher->update(rcon_user_);
	hasher->update(packet.salt.data(), packet.salt.size());
	hasher->update(salt_.data(), salt_.size());
	hasher->update(sess_key_.t.data(), sess_key_.t.size());
	auto res = hasher->final();
	return std::equal(res.begin(), res.end(), std::begin(packet.proof), std::end(packet.proof));
}

} // ember