/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/util/FileMD5.h>
#include <botan/bigint.h>
#include <botan/hash.h>
#include <boost/assert.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

namespace ember::util {

std::vector<std::uint8_t> generate_md5(std::span<const std::byte> buffer) {
	auto hasher = Botan::HashFunction::create_or_throw("MD5");
	hasher->update(reinterpret_cast<const std::uint8_t*>(buffer.data()), buffer.size_bytes());
	return hasher->final_stdvec();
}

std::vector<std::uint8_t> generate_md5(const std::string& file) {
	std::ifstream stream(file, std::ios::in | std::ios::binary);

	if(!stream) {
		throw std::runtime_error("Could not open file for MD5, " + file);
	}

	auto remaining = std::filesystem::file_size(file);
	auto hasher = Botan::HashFunction::create_or_throw("MD5");
	std::size_t block_size = hasher->hash_block_size();
	std::vector<char> buffer(block_size);

	while(remaining) {
		std::size_t read_size = remaining >= block_size? block_size : remaining;
		stream.read(buffer.data(), read_size);
		hasher->update(reinterpret_cast<const std::uint8_t*>(buffer.data()), read_size);
		remaining -= read_size;

		if(!stream.good()) {
			throw std::runtime_error("Could not read file for MD5, " + file);
		}
	}

	return hasher->final_stdvec();
}

} // util, ember