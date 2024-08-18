/*
 * Copyright (c) 2014 - 2024 Ember
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
		_256_BIT, _1024_BIT,
		_1536_BIT, _2048_BIT,
		_3072_BIT, _4096_BIT,
		_6144_BIT, _8192_BIT
	};

	Generator(Botan::BigInt g, Botan::BigInt N)
		: g_(std::move(g)), N_(std::move(N)) {}
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