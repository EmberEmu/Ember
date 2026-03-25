/*
 * Copyright (c) 2014 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Generator.h>
#include <srp6/detail/Primes.h>
#include <boost/assert.hpp>
#include <exception>

namespace ember::srp6 {

Botan::BigInt Generator::g_from_group(const Group group) {
	switch(group) {
		case Group::g_256_bit:
			return 7;
		case Group::g_1024_bit:
			[[fallthrough]];
		case Group::g_1536_bit:
			[[fallthrough]];
		case Group::g_2048_bit:
			return 2;
		case Group::g_3072_bit:
			[[fallthrough]];
		case Group::g_4096_bit:
			[[fallthrough]];
		case Group::g_6144_bit:
			return 5;
		case Group::g_8192_bit:
			return 19;
	}

	BOOST_ASSERT_MSG(0, "Unhandled enum constant - this is an SRP6 library error!");
	std::terminate();
}

Botan::BigInt Generator::n_from_group(const Group group) {
	switch(group) {
		case Group::g_256_bit:
			return Botan::BigInt(g_256_bit);
		case Group::g_1024_bit:
			return Botan::BigInt(g_1024_bit);
		case Group::g_1536_bit:
			return Botan::BigInt(g_1536_bit);
		case Group::g_2048_bit:
			return Botan::BigInt(g_2048_bit);
		case Group::g_3072_bit:
			return Botan::BigInt(g_3072_bit);
		case Group::g_4096_bit:
			return Botan::BigInt(g_4096_bit);
		case Group::g_6144_bit:
			return Botan::BigInt(g_6144_bit);
		case Group::g_8192_bit:
			return Botan::BigInt(g_8192_bit);
	}

	BOOST_ASSERT_MSG(0, "Unhandled enum constant - this is an SRP6 library error!");
	std::terminate();
}

Generator::Generator(const Group group)
	: g_(g_from_group(group)),
	  N_(n_from_group(group)) { }

} // srp6, ember