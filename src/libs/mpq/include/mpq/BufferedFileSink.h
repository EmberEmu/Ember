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
#include <filesystem>
#include <vector>
#include <cstdio>

namespace ember::mpq {

class BufferedFileSink final : public ExtractionSink {
	std::FILE* file_ = nullptr;
	std::vector<std::byte> buffer_;
	std::size_t offset_ = 0;

	void store(std::span<const std::byte> data) override {
		const auto free = buffer_.size() - offset_;

		if(data.size_bytes() > free) {
			throw exception("extraction: buffer out of space");
		}

		std::memcpy(buffer_.data() + offset_, data.data(), data.size_bytes());
		offset_ += data.size_bytes();
	}

public:
	BufferedFileSink(std::filesystem::path path, std::size_t size) : buffer_(size) {
		if(path.has_parent_path()) {
			std::filesystem::create_directories(path.parent_path());
		}

		file_ = std::fopen(path.string().c_str(), "wb");

		if(!file_) {
			throw exception("extraction: unable to create file");
		}
	}

	void operator()(std::span<const std::byte> data) override {
		store(data);
	}

	void flush() {
		const auto res = std::fwrite(buffer_.data(), buffer_.size(), 1, file_);

		if(res != 1) {
			throw exception("extraction: file writing failed");
		}
	}

	~BufferedFileSink() {
		std::fclose(file_);
	}
};

} // mpq, ember