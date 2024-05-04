/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/ExtractionSink.h>
#include <mpq/Exception.h>
#include <mpq/SharedDefs.h>
#include <boost/container/small_vector.hpp>
#include <cstddef>
#include <cstring>

namespace ember::mpq {

class DynamicMemorySink final : public ExtractionSink {
	boost::container::small_vector<std::byte, LIKELY_SECTOR_SIZE> buffer_;
	std::size_t offset_ = 0;

	void store(std::span<const std::byte> data) {
		const auto free_space = buffer_.size() - offset_;

		if(data.size_bytes() > free_space) {
			buffer_.resize(buffer_.size() + data.size_bytes());
		}

		std::memcpy(buffer_.data() + offset_, data.data(), data.size_bytes());
		offset_ += data.size_bytes();
	}

public:
	void operator()(std::span<const std::byte> data) override {
		store(data);
	}

	std::size_t size() const override {
		return offset_;
	}

	auto data() {
		return std::span(buffer_.data(), offset_);
	}
};

} // mpq, ember