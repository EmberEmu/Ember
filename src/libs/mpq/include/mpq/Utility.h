/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/MPQ.h>
#include <optional>
#include <span>
#include <cstdint>

namespace ember::mpq {

std::optional<std::uint32_t> key_recover(std::span<const std::uint32_t> sectors,
                                         std::uint32_t block_size);

} // mpq, ember