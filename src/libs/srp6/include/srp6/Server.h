/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <srp6/Util.h>
#include <srp6/Generator.h>
#include <srp6/Exception.h>
#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <cstddef>

namespace ember::srp6 {

class Server final {
	static inline thread_local Botan::AutoSeeded_RNG rng;

	const Botan::BigInt v_, N_, b_;
	Botan::BigInt B_, k_{ 3 };

public:
	Server(const Generator& gen, Botan::BigInt v, Botan::BigInt b, bool srp6a = false);
	Server(const Generator& gen, Botan::BigInt v, std::size_t key_size = 32, bool srp6a = false);
	inline const Botan::BigInt& public_ephemeral() const { return B_; }
	SessionKey session_key(const Botan::BigInt& A, Compliance mode = Compliance::GAME,
	                       bool interleave_override = false);
	Botan::BigInt generate_proof(const SessionKey& key, const Botan::BigInt& A,
	                             const Botan::BigInt& client_proof) const;
};

} //srp6, ember