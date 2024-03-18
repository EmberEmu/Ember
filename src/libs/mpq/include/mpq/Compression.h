/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <expected>
#include <span>
#include <cstddef>

namespace ember::mpq {

std::expected<std::size_t, int> decompress_pklib(std::span<const std::byte> input,
                                                 std::span<std::byte> output);

std::expected<std::size_t, int> decompress(std::span<const std::byte> input,
										   std::span<std::byte> output);

} // mpq, ember