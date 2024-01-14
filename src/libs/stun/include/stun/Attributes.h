/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Protocol.h>
#include <array>
#include <variant>
#include <vector>
#include <cstdint>

namespace ember::stun::attributes {

namespace detail {
	struct Address_RFC3489 {
		std::uint32_t ipv4;
		std::uint16_t port;
	};

	struct Address_RFC5389 {
		AddressFamily family;
		std::uint32_t ipv4;
		std::array<std::uint32_t, 4> ipv6;
		std::uint16_t port;
	};
} // detail

using MappedAddress = detail::Address_RFC5389;
using XorMappedAddress = detail::Address_RFC5389;
using ChangedAddress = detail::Address_RFC3489;
using SourceAddress = detail::Address_RFC3489;
using ReflectedFrom = detail::Address_RFC3489;

// "variable length opaque value"
class Username {
	std::vector<std::uint8_t> username;
};

class Password {
	std::vector<std::uint8_t> password;
};

using Attribute = std::variant<
	MappedAddress
	//XorMappedAddress,
	//ChangedAddress,
	//SourceAddress,
	//ReflectedFrom,
	//Username,
	//Password
>;

// todo
//0x0008 : MESSAGE - INTEGRITY
//0x0009 : ERROR - CODE
//0x000A : UNKNOWN - ATTRIBUTES
//0x0014 : REALM
//0x0015 : NONCE
//0x8022: SOFTWARE
//0x8023 : ALTERNATE - SERVER
//0x8028 : FINGERPRINT

} // stun, ember