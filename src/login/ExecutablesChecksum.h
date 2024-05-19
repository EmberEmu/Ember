/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/secmem.h>
#include <array>
#include <span>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::client_integrity {

std::array<std::uint8_t, 20> checksum(std::span<const std::uint8_t> seed,
                                      std::span<const std::byte> buffer);
std::array<std::uint8_t, 20> finalise(std::span<const std::uint8_t> checksum,
                                      std::span<const std::uint8_t> seed);

} // client_integrity, ember