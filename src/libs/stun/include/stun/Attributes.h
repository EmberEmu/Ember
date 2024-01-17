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
#include <string>
#include <variant>
#include <vector>
#include <cstdint>
#include <cstddef>

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
struct AlternateServer { IP_BOTH };
struct XorMappedAddress { IP_BOTH };
struct ChangedAddress { IPV4_ONLY };
struct ReflectedFrom { IPV4_ONLY };
struct SourceAddress { IPV4_ONLY };
struct ResponseOrigin { IP_BOTH };
struct OtherAddress { IP_BOTH };
struct ResponseAddress { IPV4_ONLY };

// "variable length opaque value"
struct Username {
	std::vector<std::uint8_t> username;
};

struct Password {
	std::vector<std::uint8_t> password;
};

struct UnknownAttributes {
	std::vector<Attributes> attributes;
};

struct ErrorCode {
	std::uint32_t code;
	std::string reason;
};

struct MessageIntegrity {
	std::array<std::byte, 20> hmac_sha1;
};

struct MessageIntegrity256 {
	std::array<std::byte, 32> hmac_sha256;
};

struct Software {
	std::string description;
};

struct Fingerprint {
	std::uint32_t crc32;
};

using Attribute = std::variant<
	MappedAddress,
	XorMappedAddress,
	ResponseOrigin,
	OtherAddress,
	ChangedAddress,
	SourceAddress,
	ReflectedFrom,
	ResponseAddress,
	UnknownAttributes,
	MessageIntegrity,
	MessageIntegrity256,
	ErrorCode,
	Username,
	Password,
	Software,
	AlternateServer,
	Fingerprint
>;

} // stun, ember