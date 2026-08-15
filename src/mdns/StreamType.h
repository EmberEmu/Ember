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
#include <span>
#include <vector>

namespace ember::dns {

using ConstBufferAdaptor = spark::io::BufferAdaptor<std::span<const std::uint8_t>>;

using StreamReadBigEndian = spark::io::BinaryStream<
	ConstBufferAdaptor,
	spark::io::allow_throw_t,
	spark::io::endian::as_big_t
>;

using BufferAdaptor = spark::io::BufferAdaptor<std::vector<std::uint8_t>>;

using StreamWriteBigEndian = spark::io::BinaryStream<
	BufferAdaptor,
	spark::io::allow_throw_t,
	spark::io::endian::as_big_t
>;

};