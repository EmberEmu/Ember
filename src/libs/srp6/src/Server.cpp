/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Server.h>
#include <botan/numthry.h>
#include <botan/rng.h>
#include <utility>

using Botan::BigInt;
using Botan::power_mod;

namespace ember::srp6 {

Server::Server(const Generator& gen, BigInt v, BigInt b, bool srp6a)
              : v_(std::move(v)), N_(gen.prime()), b_(std::move(b)) {
	if(srp6a) {
		k_ = std::move(detail::compute_k(gen.generator(), N_));
	}

	B_ = (k_ * v_ + gen(b_)) % N_;
}

Server::Server(const Generator& gen, BigInt v, std::size_t key_size, bool srp6a)
               : Server(gen, std::move(v), BigInt(rng, key_size * 8) % gen.prime(),
                 srp6a) { }

SessionKey Server::session_key(BigInt A, Compliance mode, bool interleave_override) {
	bool interleave = (mode == Compliance::GAME);
	
	if(interleave_override) {
		interleave = !interleave;
	}

	if(!(A % N_) || A < 0) {
		throw exception("Client's ephemeral key is invalid!");
	}

	BigInt u = detail::scrambler(A, B_, N_.bytes(), mode);
	BigInt S = power_mod(A * power_mod(v_, u, N_), b_, N_);
	A_ = std::move(A);

	if(interleave) {
		return SessionKey(detail::interleaved_hash(detail::encode_flip_1363(S, N_.bytes())));
	} else {
		KeyType key;
		key.resize(N_.bytes());
		S.binary_encode(key.data(), key.size());
		return SessionKey(key);
	}
}

BigInt Server::generate_proof(const SessionKey& key, const BigInt& client_proof) const {
	return generate_server_proof(A_, client_proof, key, N_.bytes());
}

} //srp6, ember