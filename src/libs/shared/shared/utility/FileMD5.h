/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/utility/HashDefines.h>
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::utility {

std::array<std::uint8_t, hash_sizes::md5> generate_md5(std::span<const std::byte> data);
std::array<std::uint8_t, hash_sizes::md5> generate_md5(const std::filesystem::path& file);

} // utility, ember