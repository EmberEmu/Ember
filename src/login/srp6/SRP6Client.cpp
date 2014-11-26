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
               : identifier_(std::move(identifier)), password_(std::move(password)),
			     gen_(std::move(gen)) {
	a_ = BigInt::decode((AutoSeeded_RNG()).random_vec(key_size)) % gen_.prime();
	A_ = gen(a_) /* % N */;
}

SessionKey Client::session_key(const BigInt& B, const SecureVector<Botan::byte>& salt) {
	if(!(B % gen_.prime()) || B < 0) {
		throw SRP6::exception("Server's ephemeral key is invalid!");
	}

	B_ = B;
	salt_ = salt;

	BigInt u = detail::scrambler(A_, B);

	if (u <= 0) {
		throw SRP6::exception("Scrambling parameter <= 0");
	}

	BigInt x = detail::compute_x(identifier_, password_, salt);
	BigInt S = power_mod((B - k_ * gen_(x)) % gen_.prime(), a_ + u * x, gen_.prime());
	return SessionKey(detail::interleaved_hash(detail::encode_flip(S)));
}

BigInt Client::generate_proof(const SessionKey& key) const {
	return generate_client_proof(identifier_, key, gen_.prime(), gen_.generator(), A_, B_, salt_);
}

} //SRP6