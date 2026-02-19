/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/bigint.h>
#include <botan/numthry.h>
#include <utility>

namespace ember::srp6 {

struct Generator {
	enum class Group {
		g_256_bit, g_1024_bit,
		g_1536_bit, g_2048_bit,
		g_3072_bit, g_4096_bit,
		g_6144_bit, g_8192_bit
	};

	Generator(Botan::BigInt g, Botan::BigInt N)
		: g_(std::move(g)),
		  N_(std::move(N)) {}

	explicit Generator(Group group);

	inline const Botan::BigInt& prime() const { return N_; }
	inline const Botan::BigInt& generator() const { return g_; }

	inline Botan::BigInt operator()(const Botan::BigInt& x) const {
		return Botan::power_mod(g_, x, N_);
	}

private:
	Botan::BigInt g_from_group(Generator::Group& group);
	Botan::BigInt n_from_group(Generator::Group& group);

	Botan::BigInt g_, N_;
};

} // srp6, ember