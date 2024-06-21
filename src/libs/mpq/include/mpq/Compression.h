/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/DecompressionError.h>
#include <expected>
#include <span>
#include <cstddef>
#include <cstdint>

namespace ember::mpq {

int next_compression(std::uint8_t& mask);

std::expected<std::size_t, int> decompress_pklib(std::span<const std::byte> input,
                                                 std::span<std::byte> output);

std::expected<std::size_t, DecompressionError>
decompress(std::span<const std::byte> input,
           std::span<std::byte> output,
           int def_comp = -1);

} // mpq, ember