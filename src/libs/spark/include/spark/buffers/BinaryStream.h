/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BinaryStreamReader.h>
#include <spark/buffers/BinaryStreamWriter.h>
#include <spark/buffers/StreamBase.h>
#include <spark/buffers/Buffer.h>
#include <spark/Exception.h>
#include <algorithm>
#include <string>
#include <cstddef>
#include <cstring>

namespace ember::spark {

class BinaryStream final : public BinaryStreamReader, public BinaryStreamWriter {
public:
	explicit BinaryStream(Buffer& source, std::size_t read_limit = 0)
                          : BinaryStreamReader(source, read_limit), BinaryStreamWriter(source),
	                        StreamBase(source) {}
	~BinaryStream() = default;
};

} // spark, ember