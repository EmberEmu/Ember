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

Botan::BigInt Generator::g_from_group(Group& group) {
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

// todo: Botan 3 should have std::span overloads to clean this up
Botan::BigInt Generator::n_from_group(Group& group) {
	switch(group) {
		case Group::g_256_bit:
			return { g_256_bit.data(), g_256_bit.size() };
		case Group::g_1024_bit:
			return { g_1024_bit.data(), g_1024_bit.size() };
		case Group::g_1536_bit:
			return { g_1536_bit.data(), g_1536_bit.size() };
		case Group::g_2048_bit:
			return { g_2048_bit.data(), g_2048_bit.size() };
		case Group::g_3072_bit:
			return { g_3072_bit.data(), g_3072_bit.size() };
		case Group::g_4096_bit:
			return { g_4096_bit.data(), g_4096_bit.size() };
		case Group::g_6144_bit:
			return { g_6144_bit.data(), g_6144_bit.size() };
		case Group::g_8192_bit:
			return { g_8192_bit.data(), g_8192_bit.size() };
	}

	BOOST_ASSERT_MSG(0, "Unhandled enum constant - this is an SRP6 library error!");
	std::terminate();
}

Generator::Generator(Group group)
	: g_(g_from_group(group)),
	  N_(n_from_group(group)) { }

} // srp6, ember