/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferBase.h>
#include <cstddef>

namespace ember::spark {

enum class SeekDir {
	SD_START, SD_BACK, SD_FORWARD
};

class BufferWrite : virtual public BufferBase {
public:
	virtual ~BufferWrite() = default;
	virtual void write(const void* source, std::size_t length) = 0;
	virtual void reserve(std::size_t length) = 0;
	virtual bool can_write_seek() const = 0;
	virtual void write_seek(SeekDir direction, std::size_t offset) = 0;
	virtual std::byte& operator[](std::size_t index) = 0;
};

} // spark, ember