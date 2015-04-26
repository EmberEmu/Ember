/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <srp6/Server.h>
#include <botan/numthry.h>
#include <botan/sha160.h>
#include <botan/auto_rng.h>
#include <botan/rng.h>

using Botan::BigInt;
using Botan::power_mod;
using Botan::AutoSeeded_RNG;

namespace ember { namespace srp6 {

Server::Server(const Generator& gen, const BigInt& v, const BigInt& b, bool srp6a)
              : v_(v), N_(gen.prime()), b_(b) {
	if(srp6a) {
		k_.swap(detail::compute_k(gen.generator(), N_));
	}

	B_ = (k_ * v_ + gen(b_)) % N_;
}

Server::Server(const Generator& gen, const BigInt& v, int key_size, bool srp6a)
               : Server(gen, v, BigInt::decode((AutoSeeded_RNG()).random_vec(key_size)) % gen.prime(),
                 srp6a) { }

SessionKey Server::session_key(const BigInt& A, bool interleave, Compliance mode) {
	if(interleave && mode == Compliance::RFC5054) {
		throw exception("Interleaving is not compliant with RFC5054!");
	}

	if(!(A % N_) || A < 0) {
		throw exception("Client's ephemeral key is invalid!");
	}

	A_ = A;
	BigInt u = detail::scrambler(A, B_, N_.bytes(), mode);
	BigInt S = power_mod(A * power_mod(v_, u, N_), b_, N_);
	return interleave? SessionKey(detail::interleaved_hash(detail::encode_flip(S)))
	                   : SessionKey(Botan::BigInt::encode(S));
}

BigInt Server::generate_proof(const SessionKey& key, const BigInt& client_proof) const {
	return generate_server_proof(A_, client_proof, key);
}

}} //srp6, ember