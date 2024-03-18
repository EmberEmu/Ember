/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/Utility.h>
#include <mpq/Crypt.h>
#include <tuple>
#include <utility>

namespace ember::mpq {

// This is ported from StormLib, credits to Ladislav Zezula
// Original source: SBaseCommon.cpp (MIT licensed)
std::optional<std::uint32_t> key_recover(std::span<const std::uint32_t> sectors,
                                         const std::uint32_t sector_size) {
	constexpr auto table = crypt_table();
	std::uint32_t expected = sectors.size_bytes();

	if(sectors.size() < 2) {
		return std::nullopt;
	}

	constexpr auto retries = 4;

	for(std::uint32_t i = expected + retries; expected < i; ++expected) {
		std::uint32_t max_value = sector_size + expected;
		std::uint32_t combined_keys = combined_keys = (sectors[0] ^ expected) - 0xEEEEEEEE;
		std::uint32_t data[2]{};

		for(std::uint32_t i = 0; i < 0x100; ++i) {
			std::uint32_t saved_key = 0;
			std::uint32_t key1 = combined_keys - table[0x400 + i];
			std::uint32_t key2 = 0xEEEEEEEE;

			key2 += table[0x400 + (key1 & 0xFF)];
			data[0] = sectors[0] ^ (key1 + key2);

			if(data[0] == expected) {
				saved_key = key1;

				key1 = ((~key1 << 0x15) + 0x11111111) | (key1 >> 0x0B);
				key2 = data[0] + key2 + (key2 << 5) + 3;

				key2 += table[0x400 + (key1 & 0xFF)];
				data[1] = sectors[1] ^ (key1 + key2);

				if(data[1] <= max_value) {
					return saved_key + 1; // key for sector decryption is -1
				}
			}
		}
	}

	return std::nullopt;
}

} // mpq, ember