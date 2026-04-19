/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SocketType.h"
#include "BuildDefines.h"
#include <protocol/MessageView.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/DynamicTLSBuffer.h>
#include <spark/buffers/StaticBuffer.h>

namespace ember::realm {

static constexpr auto inbound_size      { CLIENT_BUFFER_IN_SIZE         };
static constexpr auto outbound_size     { CLIENT_BUFFER_OUT_SIZE        };
static constexpr auto max_outbound_size { CLIENT_BUFFER_OUT_MAX_SIZE    };
static constexpr auto prealloc_nodes    { PREALLOCATED_NODES_PER_THREAD };

using StaticBuffer  = spark::io::StaticBuffer<std::uint8_t, inbound_size>;

using BinaryStream = spark::io::BinaryStream<
	StaticBuffer,
	spark::io::no_throw_t,
	spark::io::endian::as_little_t
>;

using DynamicTLSBuffer = spark::io::DynamicTLSBuffer<
	outbound_size, prealloc_nodes, spark::io::NoRefCounting, spark::io::UnsafeEntrant
>;

template<typename _ty>
using message_view = protocol::message_view<_ty, BinaryStream>;

} // realm, ember