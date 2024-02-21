/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Protocol.h>
#include <algorithm>
#include <array>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::stun::attributes {

// can't be bothered with strong typedefs to reduce duplication here,
// which would be needed to differentiate between them in the variant
#define STRUCT_IP_BOTH(name)           \
struct name {                          \
	std::uint32_t ipv4;                \
	std::array<std::uint8_t, 16> ipv6; \
	std::uint16_t port;                \
	AddressFamily family;              \
};                                     \
                                       \
inline bool operator==(const name& lhs, const name& rhs) { \
	return (lhs.family == rhs.family                       \
		&& lhs.ipv4 == rhs.ipv4 && lhs.port == rhs.port    \
		&& std::equal(lhs.ipv6.begin(), lhs.ipv6.end(),    \
		              rhs.ipv6.begin(), rhs.ipv6.end()));  \
}

#define STRUCT_IPV4(name)       \
struct name {                   \
	std::uint32_t ipv4;         \
	std::uint16_t port;         \
	AddressFamily family;       \
};                              \
                                \
inline bool operator==(const name& lhs, const name& rhs) { \
	return (lhs.family == rhs.family                       \
		&& lhs.ipv4 == rhs.ipv4                            \
		&& lhs.port == rhs.port);                          \
}

STRUCT_IP_BOTH(MappedAddress)
STRUCT_IP_BOTH(AlternateServer)
STRUCT_IP_BOTH(XorMappedAddress)
STRUCT_IPV4(ChangedAddress)
STRUCT_IPV4(ReflectedFrom)
STRUCT_IPV4(SourceAddress)
STRUCT_IP_BOTH(ResponseOrigin)
STRUCT_IP_BOTH(OtherAddress)
STRUCT_IPV4(ResponseAddress)

// "variable length opaque value"
struct Username {
	std::vector<std::uint8_t> value;
};

struct Password {
	std::vector<std::uint8_t> value;
};

struct UnknownAttributes {
	std::vector<Attributes> attributes;
};

struct ErrorCode {
	std::uint32_t code;
	std::string reason;
};

struct MessageIntegrity {
	std::array<std::uint8_t, 20> hmac_sha1;
};

struct MessageIntegrity256 {
	std::array<std::uint8_t, 32> hmac_sha256;
};

struct Software {
	std::string value;
};

struct Fingerprint {
	std::uint32_t crc32;
};

struct Realm {
	std::string value;
};

struct Nonce {
	std::string value;
};

struct Padding {
	std::string value;
};

struct Priority {
	std::uint32_t value;
};

struct IceControlled {
	std::uint64_t value;
};

struct IceControlling {
	std::uint64_t value;
};

struct UseCandidate {};

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
	Fingerprint,
	Realm,
	Nonce,
	Padding,
	Priority,
	IceControlled,
	IceControlling,
	UseCandidate
>;

} // stun, ember