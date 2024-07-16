/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "DNSDefines.h"
#include "detail/Parser.h"
#include <spark/buffers/pmr/BinaryStream.h>
#include <shared/smartenum.hpp>
#include <expected>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <utility>
#include <cstddef>

namespace ember::dns {

std::expected<Query, parser::Result> deserialise(std::span<const std::uint8_t> buffer);
void serialise(const Query& query, spark::io::pmr::BinaryStream& stream);

} // dns, ember