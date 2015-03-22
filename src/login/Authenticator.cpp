/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authenticator.h"
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

auto LoginAuthenticator::proof_check(protocol::ClientLoginProof* proof) -> ProofResult {
	// Usernames aren't required to be uppercase in the DB but the client requires it for calculations
	std::string user_upper(user_.username());
	std::transform(user_upper.begin(), user_upper.end(), user_upper.begin(), ::toupper);

	// Need to reverse the order thanks to Botan's big-endian buffers
	std::reverse(std::begin(proof->A), std::end(proof->A));
	std::reverse(std::begin(proof->M1), std::end(proof->M1));

	Botan::BigInt A(proof->A, sizeof(proof->A));
	Botan::BigInt M1(proof->M1, sizeof(proof->M1));
	srp6::SessionKey key(srp_->session_key(A, true, srp6::Compliance::GAME));

	Botan::BigInt B = srp_->public_ephemeral();
	Botan::BigInt M1_S = srp6::generate_client_proof(user_upper, key, gen_.prime(), gen_.generator(),
	                                                 A, B, Botan::BigInt(user_.salt()));
	sess_key_ = key;
	return {M1 == M1_S, srp_->generate_proof(key, M1)};
}

std::string LoginAuthenticator::session_key() {
	Botan::BigInt key(Botan::BigInt::decode(sess_key_));
	std::stringstream keystr; keystr << key;
	return keystr.str();
}

ReconnectAuthenticator::ReconnectAuthenticator(const std::string& username, const std::string& session_key,
                                               const Botan::SecureVector<Botan::byte>& bytes) {
	rcon_chall_ = bytes;
	rcon_user_ = username;
	sess_key_ = Botan::BigInt::encode(Botan::BigInt(session_key));
}

bool ReconnectAuthenticator::proof_check(const protocol::ClientReconnectProof* proof) {
	Botan::BigInt client_proof(proof->R1, sizeof(proof->R1));

	Botan::SHA_160 hasher;
	hasher.update(rcon_user_);
	hasher.update(Botan::BigInt::encode(client_proof));
	hasher.update(rcon_chall_);
	hasher.update(sess_key_);
	auto res = hasher.final();
	
	// todo - change this to include std::end for R2 once on VS2015 RTM (C++14)
	return std::equal(res.begin(), res.end(), std::begin(proof->R2));
}

} // ember