/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <spark/buffers/DynamicBuffer.h>
#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <array>
#include <span>
#include <cstdint>

namespace ember {

static constexpr std::uint16_t INBOUND_SIZE  { 1024 };
static constexpr std::uint16_t OUTBOUND_SIZE { 2048 };

using AdaptorInType = spark::v2::BufferAdaptor<std::span<std::uint8_t>>;
using BufferInType = std::array<std::uint8_t, INBOUND_SIZE>;
using BufferOutType = spark::DynamicBuffer<OUTBOUND_SIZE>;

using ClientStream = spark::v2::BinaryStream<AdaptorInType>;

} // ember