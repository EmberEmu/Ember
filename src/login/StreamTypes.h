/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferAdaptor.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/DynamicBuffer.h>
#include <vector>

namespace ember {

constexpr auto buffer_node_size = 1024u;

using BufferType = spark::io::DynamicBuffer<buffer_node_size>;
using PacketStream = spark::io::BinaryStream<BufferType, spark::io::no_throw_t, spark::io::endian::as_little_t>;

} // ember