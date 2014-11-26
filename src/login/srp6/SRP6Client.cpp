/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SRP6Client.h"
#include <botan/numthry.h>
#include <botan/secmem.h>
#include <botan/auto_rng.h>
#include <botan/rng.h>

namespace SRP6 {

using Botan::BigInt;
using Botan::SecureVector;
using Botan::power_mod;
using Botan::AutoSeeded_RNG;

Client::Client(std::string identifier, std::string password, Generator gen, int key_size)
               : identifier(std::move(identifier)), password(std::move(password)),
			     gen(std::move(gen)) {
	a = BigInt::decode((AutoSeeded_RNG()).random_vec(key_size)) % this->gen.prime();
	A = gen(a) /* % N */;
}

SessionKey Client::session_key(const BigInt& B, const SecureVector<Botan::byte>& salt) {
	if(!(B % gen.prime()) || B < 0) {
		throw SRP6::exception("Server's ephemeral key is invalid!");
	}

	this->B = B;
	this->salt = salt;

	BigInt u = detail::scrambler(A, B);

	if (u <= 0) {
		throw SRP6::exception("Scrambling parameter <= 0");
	}

	BigInt x = detail::compute_x(identifier, password, salt);
	BigInt S = power_mod((B - k * gen(x)) % gen.prime(), a + u * x, gen.prime());
	return SessionKey(detail::interleaved_hash(detail::encode_flip(S)));
}

BigInt Client::generate_proof(const SessionKey& key) const {
	return generate_client_proof(identifier, key, gen.prime(), gen.generator(), A, B, salt);
}

} //SRP6