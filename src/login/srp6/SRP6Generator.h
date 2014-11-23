/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/log/trivial.hpp>
#include <botan/bigint.h>
#include <botan/numthry.h>

namespace SRP6 {

struct Generator {
	explicit Generator(const Botan::BigInt& g, const Botan::BigInt& N) : g_(g), N_(N) {}
	inline Botan::BigInt operator()(const Botan::BigInt& x) const {
		return Botan::power_mod(g_, x, N_);
	}

private:
	const Botan::BigInt g_, N_;
};

} //SRP6