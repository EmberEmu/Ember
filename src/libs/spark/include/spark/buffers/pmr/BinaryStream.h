/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BinaryStreamReader.h>
#include <spark/buffers/pmr/BinaryStreamWriter.h>
#include <spark/buffers/pmr/StreamBase.h>
#include <spark/buffers/pmr/Buffer.h>
#include <cstddef>

namespace ember::spark::io::pmr {

class BinaryStream final : public BinaryStreamReader, public BinaryStreamWriter {
public:
	explicit BinaryStream(Buffer& source, std::size_t read_limit = 0)
		: StreamBase(source),
		  BinaryStreamReader(source, read_limit),
		  BinaryStreamWriter(source) {}

	explicit BinaryStream(Buffer& source, no_throw_t)
		: StreamBase(source, false),
		BinaryStreamReader(source, no_throw),
		BinaryStreamWriter(source, no_throw) {}


	explicit BinaryStream(Buffer& source, std::size_t read_limit, no_throw_t)
		: StreamBase(source, false),
		  BinaryStreamReader(source, read_limit, no_throw),
		  BinaryStreamWriter(source, no_throw) {}

	~BinaryStream() override = default;
};

} // pmr, io, spark, ember