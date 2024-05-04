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
#include <cstdio>

namespace ember::mpq {

class FileSink final : public ExtractionSink {
	std::FILE* file_ = nullptr;
	std::size_t size_ = 0;

	void store(std::span<const std::byte> data) {
		const auto res = std::fwrite(data.data(), data.size_bytes(), 1, file_);

		if(res != 1) {
			throw exception("extraction: file writing failed");
		}

		size_ += data.size_bytes();
	}

public:
	FileSink(std::filesystem::path path) {
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

	std::size_t size() const override {
		return size_;
	}

	~FileSink() {
		std::fclose(file_);
	}

	FileSink(const FileSink&) = delete;
	FileSink(FileSink&&) = delete;
	FileSink& operator=(FileSink&&) = delete;
	FileSink& operator=(FileSink&) = delete;
};

} // mpq, ember