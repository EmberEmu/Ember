/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/CryptTables.h>
#include <shared/utility/polyfill/start_lifetime_as>
#include <span>
#include <type_traits>
#include <cstddef>
#include <cstdint>

// Thanks to https://www.zezula.net/en/mpq/techinfo.html for these time-savers
// Same functions, just modernised a bit

namespace ember::mpq {

static constexpr void decrypt_block(std::span<std::byte> buffer, std::uint32_t key) {
	std::uint32_t seed = 0xEEEEEEEE;
	std::span<std::uint32_t> cast_block { 
		std::start_lifetime_as<std::uint32_t>(buffer.data()), buffer.size_bytes() >> 2 
	};

	for(auto& block : cast_block) {
		seed += crypt_table[0x400 + (key & 0xFF)];
		const auto ch = block ^ (key + seed);
		key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
		seed = ch + seed + (seed << 5) + 3;
		block = ch;
	}
}

static constexpr std::uint32_t hash_string(const std::string_view key, std::uint32_t type) {
	std::uint32_t seed1 = 0x7FED7FED;
	std::uint32_t seed2 = 0xEEEEEEEE;

	for(auto byte : key) {
		auto ch = toupper_table[byte];
		seed1 = crypt_table[(type << 8) + ch] ^ (seed1 + seed2);
		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
	}

	return seed1;
}

} // v0, mpq, ember