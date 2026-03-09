/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/HashDefines.h>
#include <array>
#include <span>
#include <string_view>
#include <cstdint>

namespace ember::realm::digest {

std::array<std::uint8_t, hash_sizes::sha160> calculate(std::span<const std::uint8_t> key,
                                                       const std::string_view username,
                                                       std::uint32_t protocol_id,
                                                       std::uint32_t client_seed,
                                                       std::uint32_t server_seed);

} // digest, realm, ember