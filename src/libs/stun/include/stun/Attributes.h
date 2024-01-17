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

// can't be bothered with strong typedefs to reduce duplication here,
// which would be needed to differentiate between them in the variant
#define IP_BOTH                        \
	AddressFamily family;              \
	std::uint32_t ipv4;                \
	std::array<std::uint32_t, 4> ipv6; \
	std::uint16_t port;

#define IPV4_ONLY                      \
	AddressFamily family;              \
	std::uint32_t ipv4;                \
	std::uint16_t port;

struct MappedAddress { IP_BOTH };
struct XorMappedAddress { IP_BOTH };
struct ChangedAddress { IPV4_ONLY };
struct ReflectedFrom { IPV4_ONLY };
struct SourceAddress { IPV4_ONLY };
struct ResponseOrigin { IP_BOTH };
struct OtherAddress { IP_BOTH };

// "variable length opaque value"
class Username {
	std::vector<std::uint8_t> username;
};

class Password {
	std::vector<std::uint8_t> password;
};

using Attribute = std::variant<
	MappedAddress,
	XorMappedAddress,
	ResponseOrigin,
	OtherAddress,
	ChangedAddress,
	SourceAddress,
	ReflectedFrom,
	Username,
	Password
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