/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Survey.h"
#include <shared/util/FileMD5.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>
#include <cassert>
#include <stdexcept>

namespace ember {

// todo, only supports x86 Windows for the time being
void Survey::add_data(grunt::Platform, grunt::System, const std::string& path) {
	std::ifstream file(path, std::ios::binary);

	if(!file) {
		throw std::runtime_error("Error opening " + path);
	}

	FileMeta fmeta {
		.name = "Survey",
		.size = std::filesystem::file_size(path)
	};

	std::vector<std::byte> buffer(static_cast<std::size_t>(fmeta.size));
	file.read(reinterpret_cast<char*>(buffer.data()), fmeta.size);

	if(!file.good()) {
		throw std::runtime_error("An error occured while reading " + path);
	}

	const auto md5 = util::generate_md5(buffer);
	const auto md5_bytes = std::as_bytes(std::span(md5));
	static_assert(md5.size() == fmeta.md5.size());
	std::copy(md5_bytes.begin(), md5_bytes.end(), file_.md5.data());

	file_ = std::move(fmeta);
	data_ = std::move(buffer);
}

// todo, only supports x86 Windows for the time being
FileMeta Survey::meta(const grunt::Platform platform, const grunt::System os) const {
	return file_;
}

// todo, only supports x86 Windows for the time being
std::optional<std::span<const std::byte>> Survey::data(const grunt::Platform platform,
                                                       const grunt::System os) const {
	if(platform != grunt::Platform::x86 || os != grunt::System::Win) {
		return std::nullopt;
	}

	if(data_.empty()) {
		return std::nullopt;
	}

	return data_;
}

std::uint32_t Survey::id() const {
	return id_;
}

} // ember