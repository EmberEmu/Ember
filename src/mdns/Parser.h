/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "DNSDefines.h"
#include <shared/smartenum.hpp>
#include <span>
#include <cstddef>

namespace ember::dns {

smart_enum_class(Result, std::uint8_t,
	OK, HEADER_TOO_SMALL, PAYLOAD_TOO_LARGE
);

class Parser final {
public:
    static Result validate(std::span<const std::byte> buffer);
    static Flags decode_flags(std::uint16_t flags);
	static std::uint16_t encode_flags(Flags flags);
    static const Header* header_overlay(std::span<const std::byte> buffer);
};

} // dns, ember