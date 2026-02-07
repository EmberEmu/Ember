/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <type_traits>
#include <cstdint>

// Thanks to https://www.zezula.net/en/mpq/techinfo.html for these time-savers
// Same functions, just modernised a bit

namespace ember::mpq {

[[nodiscard]] static consteval auto crypt_table() {
    std::array<std::uint32_t, 1280> table{};
    std::uint32_t seed = 0x00100001;

    for(std::uint32_t index1 = 0; index1 < 0x100; ++index1) { 
        for(std::uint32_t i = 0, index2 = index1; i < 5; ++i, index2 += 0x100) { 
            seed = (seed * 125 + 3) % 0x2AAAAB;
            std::uint32_t val1 = (seed & 0xFFFF) << 0x10;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            std::uint32_t val2 = (seed & 0xFFFF);
            table[index2] = val1 | val2; 
        }
    } 

    return table;
}

extern const std::invoke_result<decltype(&crypt_table)>::type CRYPT_TABLE;
extern const std::array<char, 256> TOUPPER_TABLE;

} // mpq, ember