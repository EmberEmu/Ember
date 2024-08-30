/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Client.h>
#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/rng.h>
#include <utility>

using Botan::BigInt;
using Botan::power_mod;
using Botan::AutoSeeded_RNG;

namespace ember::srp6 {

Client::Client(std::string identifier, std::string password, Generator gen, std::size_t key_size, bool srp6a)
               : Client(std::move(identifier), std::move(password), gen,
                 BigInt::decode((AutoSeeded_RNG()).random_vec(key_size)) % gen.prime(), srp6a) { }

Client::Client(std::string identifier, std::string password, Generator gen, BigInt a, bool srp6a)
               : gen_(std::move(gen)),
                 a_(a),
                 A_(gen_(a_)) /* % N */,
                 identifier_(std::move(identifier)),
                 password_(std::move(password)) {
	if(srp6a) {
		k_ = detail::compute_k(gen_.generator(), gen_.prime());
	}
}

SessionKey Client::session_key(const BigInt& B, std::span<const std::uint8_t> salt, 
                               Compliance mode, bool interleave_override) const {
	bool interleave = (mode == Compliance::GAME);
	
	if(interleave_override) {
		interleave = !interleave;
	}

	const BigInt& N = gen_.prime();

	if(!(B % N) || B < 0) {
		throw exception("Server's ephemeral key is invalid!");
	}

	BigInt u = detail::scrambler(A_, B, N.bytes(), mode);

	if(u <= 0) {
		throw exception("Scrambling parameter <= 0");
	}

	BigInt x = detail::compute_x(identifier_, password_, salt, mode);
	BigInt S = power_mod((B - k_ * gen_(x)) % N, a_ + u * x, N);
	
	if(interleave) {
		return SessionKey(detail::interleaved_hash(detail::encode_flip_1363(S, N.bytes())));
	} else {
		KeyType key;
		key.resize(S.bytes(), boost::container::default_init);
		S.binary_encode(key.data(), key.size());
		return SessionKey(key);
	}
}

BigInt Client::generate_proof(const SessionKey& key, const Botan::BigInt& B,
                             std::span<std::uint8_t> salt) const {
	return generate_client_proof(identifier_, key, gen_.prime(), gen_.generator(), A_, B, salt);
}

} // srp6, ember