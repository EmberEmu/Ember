/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <span>
#include <string>
#include <vector>
#include <cstddef>

namespace ember::util {

std::vector<std::uint8_t> generate_md5(std::span<const std::byte> data);
std::vector<std::uint8_t> generate_md5(const std::string& file);

} // util, ember