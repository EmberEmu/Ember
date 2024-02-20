/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <stun/Attributes.h>
#include <stun/Protocol.h>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace Botan {
	class MessageAuthenticationCode;
} // Botan

namespace ember::stun::detail {

bool magic_cookie_present(std::span<const std::uint8_t> buffer);

Header read_header(std::span<const std::uint8_t> buffer);

std::size_t attribute_offset(std::span<const std::uint8_t> buffer, Attributes attr);

void hmac_helper(std::span<const std::uint8_t> buffer,
				 Botan::MessageAuthenticationCode* hmac,
				 const std::size_t msgi_offset);

std::uint32_t fingerprint(std::span<const std::uint8_t> buffer, bool complete);

std::vector<std::uint8_t> msg_integrity(std::span<const std::uint8_t> buffer,
                                        std::string_view password,
                                        bool complete);

std::vector<std::uint8_t> msg_integrity(std::span<const std::uint8_t> buffer,
                                        std::span<const std::uint8_t> username,
                                        std::string_view realm,
                                        std::string_view password,
                                        bool complete);

std::size_t generate_key(const TxID& tx_id, RFCMode mode);

} // detail, stun, ember