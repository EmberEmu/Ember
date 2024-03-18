/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/Exception.h>
#include <mpq/ExtractionSink.h>
#include <span>
#include <cstddef>
#include <cstring>

namespace ember::mpq {

class MemorySink final : public ExtractionSink {
	std::span<std::byte> buffer_;
	std::size_t offset_ = 0;

public:
	MemorySink(std::span<std::byte> buffer) : buffer_(buffer) {}

	void store(std::span<const std::byte> data) override {
		const auto free_space = buffer_.size_bytes() - offset_;

		if(data.size_bytes() > free_space) {
			throw exception("extraction: buffer out of space");
		}

		std::memcpy(buffer_.data() + offset_, data.data(), data.size_bytes());
		offset_ += data.size_bytes();
	}
};

} // mpq, ember