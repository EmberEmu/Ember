/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/bigint.h>
#include <botan/numthry.h>

namespace SRP6 {

struct Generator {
	enum class GROUP {
		_256_BIT, _1024_BIT,
		_1536_BIT, _2048_BIT,
		_3072_BIT, _4096_BIT,
		_6144_BIT, _8192_BIT
	};

	explicit Generator(const Botan::BigInt& g, const Botan::BigInt& N) : g_(g), N_(N) {}
	Generator(GROUP group);
	inline Botan::BigInt prime() const { return N_; }
	inline Botan::BigInt generator() const { return g_; }
	inline Botan::BigInt operator()(const Botan::BigInt& x) const {
		return Botan::power_mod(g_, x, N_);
	}

private:
	Botan::BigInt g_, N_;
};

} //SRP6