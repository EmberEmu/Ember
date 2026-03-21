/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SocketType.h"
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/DynamicTLSBuffer.h>
#include <spark/buffers/StaticBuffer.h>

namespace ember::realm {

static constexpr auto inbound_size  { 8192 };
static constexpr auto outbound_size { 8192 };

#if defined TARGET_PLAYER_COUNT && defined TARGET_WORKER_COUNT
static constexpr std::size_t prealloc_nodes {  TARGET_PLAYER_COUNT / TARGET_WORKER_COUNT };
#else
static constexpr std::size_t prealloc_nodes {  16 };
#endif

using StaticBuffer  = spark::io::StaticBuffer<std::uint8_t, inbound_size>;

using BinaryStream = spark::io::BinaryStream<
	StaticBuffer,
	spark::io::allow_throw_t,
	spark::io::endian::as_little_t
>;

using DynamicTLSBuffer = spark::io::DynamicTLSBuffer<
	outbound_size, prealloc_nodes, spark::io::NoRefCounting, spark::io::UnsafeEntrant
>;

} // realm, ember