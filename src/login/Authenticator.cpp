/*
 * Copyright (c) 2015 Ember
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
#include <botan/sha160.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <utility>

namespace ember {

LoginAuthenticator::LoginAuthenticator(User user) : user_(std::move(user)) {
	srp_ = std::make_unique<srp6::Server>(gen_, user_.verifier());
}

auto LoginAuthenticator::challenge_reply() -> ChallengeResponse {
	return {srp_->public_ephemeral(), Botan::BigInt(user_.salt()), gen_};
}

auto LoginAuthenticator::proof_check(const grunt::client::LoginProof* proof) -> ProofResult  try {
	// Usernames aren't required to be uppercase in the DB but the client requires it for calculations
	std::string user_upper(user_.username());
	std::transform(user_upper.begin(), user_upper.end(), user_upper.begin(), ::toupper);

	srp6::SessionKey key(srp_->session_key(proof->A));

	Botan::BigInt B = srp_->public_ephemeral();
	Botan::BigInt M1_S = srp6::generate_client_proof(user_upper, key, gen_.prime(), gen_.generator(),
	                                                 proof->A, B, Botan::BigInt(user_.salt()));
	sess_key_ = key;
	return { proof->M1 == M1_S, srp_->generate_proof(key, proof->M1) };
} catch(srp6::exception& e) {
	return { false, 0 };
}

srp6::SessionKey LoginAuthenticator::session_key() {
	return sess_key_;
}

ReconnectAuthenticator::ReconnectAuthenticator(std::string username, const Botan::BigInt& session_key,
                                               const Botan::SecureVector<Botan::byte>& bytes)
                                               : rcon_user_(std::move(username)) {
	rcon_chall_ = bytes;
	sess_key_ = Botan::BigInt::encode(session_key);
}

bool ReconnectAuthenticator::proof_check(const grunt::client::ReconnectProof* proof) {
	Botan::SHA_160 hasher;
	hasher.update(rcon_user_);
	hasher.update(proof->R1.data(), proof->R1.size());
	hasher.update(rcon_chall_);
	hasher.update(sess_key_);
	auto res = hasher.final();
	return std::equal(res.begin(), res.end(), std::begin(proof->R2), std::end(proof->R2));
}

} // ember