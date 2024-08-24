/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/util/FileMD5.h>
#include <botan/bigint.h>
#include <botan/md5/md5.h>
#include <boost/assert.hpp>
#include <filesystem>
#include <fstream>

namespace ember::util {

std::array<std::uint8_t, 16> generate_md5(std::span<const std::byte> buffer) {
	Botan::MD5 hasher;
	std::array<std::uint8_t, hasher.output_bytes> res;
	hasher.update(reinterpret_cast<const std::uint8_t*>(buffer.data()), buffer.size_bytes());
	hasher.final(res.data());
	return res;
}

std::array<std::uint8_t, 16> generate_md5(const std::string& file) {
	std::ifstream stream(file, std::ios::in | std::ios::binary);

	if(!stream) {
		throw std::runtime_error("Could not open file for MD5, " + file);
	}

	auto remaining = std::filesystem::file_size(file);
	Botan::MD5 hasher;
	std::array<std::uint8_t, hasher.output_size> res;
	std::array<char, Botan::MD5::block_bytes> buffer;

	while(remaining) {
		std::size_t read_size = remaining >= buffer.size()? buffer.size() : remaining;
		stream.read(buffer.data(), read_size);
		hasher.update(reinterpret_cast<const std::uint8_t*>(buffer.data()), read_size);
		remaining -= read_size;

		if(!stream.good()) {
			throw std::runtime_error("Could not read file for MD5, " + file);
		}
	}

	hasher.final(res.data());
	return res;
}

} // util, ember