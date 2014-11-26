/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SRP6Server.h"
#include <botan/numthry.h>
#include <botan/sha160.h>
#include <botan/auto_rng.h>
#include <botan/rng.h>

namespace SRP6 {

using Botan::BigInt;
using Botan::power_mod;
using Botan::AutoSeeded_RNG;

Server::Server(const Generator& gen, const BigInt& v, int key_size)
               : v(v), N(gen.prime()), 
                 b(BigInt::decode((AutoSeeded_RNG()).random_vec(key_size)) % gen.prime()) {
	B = (k * v + gen(b)) /* % N */;
}

SessionKey Server::session_key(const BigInt& A) {
	if(!(A % N) || A < 0) {
		throw SRP6::exception("Client's ephemeral key is invalid!");
	}

	this->A = A;
	BigInt u = detail::scrambler(A, B);
	BigInt S = power_mod(A * power_mod(v, u, N), b, N);
	return SessionKey(detail::interleaved_hash(detail::encode_flip(S)));
}

BigInt Server::generate_proof(const SessionKey& key, const BigInt& client_proof) const {
	return generate_server_proof(A, client_proof, key);
}

} //SRP6