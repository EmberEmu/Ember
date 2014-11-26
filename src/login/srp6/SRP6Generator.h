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
	explicit Generator(const Botan::BigInt& g, const Botan::BigInt& N) : g(g), N(N) {}
	inline Botan::BigInt prime() const { return N; }
	inline Botan::BigInt generator() const { return g; }
	inline Botan::BigInt operator()(const Botan::BigInt& x) const {
		return Botan::power_mod(g, x, N);
	}

private:
	const Botan::BigInt g, N;
};

} //SRP6