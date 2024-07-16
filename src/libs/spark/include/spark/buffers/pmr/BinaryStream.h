/*
 * Copyright (c) 2015 - 2024 Ember
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
#include <algorithm>
#include <string>
#include <cstddef>
#include <cstring>

namespace ember::spark::io::pmr {

class BinaryStream final : public BinaryStreamReader, public BinaryStreamWriter {
public:
	explicit BinaryStream(Buffer& source, std::size_t read_limit = 0)
                          : BinaryStreamReader(source, read_limit), BinaryStreamWriter(source),
	                        StreamBase(source) {}
	~BinaryStream() override = default;
};

} // pmr, io, spark, ember