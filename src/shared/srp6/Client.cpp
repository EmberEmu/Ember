/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <srp6/Client.h>
#include <botan/numthry.h>
#include <botan/secmem.h>
#include <botan/auto_rng.h>
#include <botan/rng.h>

using Botan::BigInt;
using Botan::SecureVector;
using Botan::power_mod;
using Botan::AutoSeeded_RNG;

namespace ember { namespace srp6 {

Client::Client(std::string identifier, std::string password, Generator gen, int key_size, bool srp6a)
               : Client(std::move(identifier), std::move(password), gen,
                 BigInt::decode((AutoSeeded_RNG()).random_vec(key_size)) % gen.prime(), srp6a) { }

Client::Client(std::string identifier, std::string password, Generator gen, BigInt a, bool srp6a)
               : identifier_(std::move(identifier)), password_(std::move(password)),
                 gen_(std::move(gen)), a_(a) {
	A_ = gen(a_) /* % N */;

	if (srp6a) {
		k_.swap(detail::compute_k(gen_.generator(), gen_.prime()));
	}
}

SessionKey Client::session_key(const BigInt& B, const SecureVector<Botan::byte>& salt, bool interleave) {
	if(!(B % gen_.prime()) || B < 0) {
		throw exception("Server's ephemeral key is invalid!");
	}

	B_ = B;
	salt_ = salt;

	BigInt u = detail::scrambler(A_, B, gen_.prime().bytes());

	if(u <= 0) {
		throw exception("Scrambling parameter <= 0");
	}

	BigInt x = detail::compute_x(identifier_, password_, salt);
	BigInt S = power_mod((B - k_ * gen_(x)) % gen_.prime(), a_ + u * x, gen_.prime());
	return interleave ? SessionKey(detail::interleaved_hash(detail::encode_flip(S)))
		: SessionKey(Botan::BigInt::encode(S));
}

BigInt Client::generate_proof(const SessionKey& key) const {
	return generate_client_proof(identifier_, key, gen_.prime(), gen_.generator(), A_, B_, salt_);
}

}} //srp6, ember