/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Generator.h>
#include <srp6/detail/Primes.h>
#include <boost/assert.hpp>

namespace ember::srp6 {

Botan::BigInt Generator::g_from_group(Group& group) {
	switch(group) {
		case Group::_256_BIT:
			return 7;
		case Group::_1024_BIT:
			[[fallthrough]];
		case Group::_1536_BIT:
			[[fallthrough]];
		case Group::_2048_BIT:
			return 2;
		case Group::_3072_BIT:
			[[fallthrough]];
		case Group::_4096_BIT:
			[[fallthrough]];
		case Group::_6144_BIT:
			return 5;
		case Group::_8192_BIT:
			return 19;
		default:
			BOOST_ASSERT_MSG(0, "Unhandled enum constant - this is an SRP6 library error!");
			return {};
	}
}

// todo: Botan 3 should have std::span overloads to clean this up
Botan::BigInt Generator::n_from_group(Group& group) {
	switch(group) {
		case Group::_256_BIT:
			return { _256_bit.data(), _256_bit.size() };
		case Group::_1024_BIT:
			return { _1024_bit.data(), _1024_bit.size() };
		case Group::_1536_BIT:
			return { _1536_bit.data(), _1536_bit.size() };
		case Group::_2048_BIT:
			return { _2048_bit.data(), _2048_bit.size() };
		case Group::_3072_BIT:
			return { _3072_bit.data(), _3072_bit.size() };
		case Group::_4096_BIT:
			return { _4096_bit.data(), _4096_bit.size() };
		case Group::_6144_BIT:
			return { _6144_bit.data(), _6144_bit.size() };
		case Group::_8192_BIT:
			return { _8192_bit.data(), _8192_bit.size() };
		default:
			BOOST_ASSERT_MSG(0, "Unhandled enum constant - this is an SRP6 library error!");
			return {};
	}
}

Generator::Generator(Group group)
	: g_(g_from_group(group)),
	  N_(n_from_group(group)) { }

} //srp6, ember