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

namespace ember::util {

std::array<std::uint8_t, 16> generate_md5(std::span<const std::byte> buffer) {
	std::array<std::uint8_t, 16> res;
	auto hasher = Botan::HashFunction::create_or_throw("MD5");
	BOOST_ASSERT_MSG(hasher->output_length() == res.size(), "Bad hash size");
	hasher->update(reinterpret_cast<const std::uint8_t*>(buffer.data()), buffer.size_bytes());
	hasher->final(res.data());
	return res;
}

std::array<std::uint8_t, 16> generate_md5(const std::string& file) {
	std::ifstream stream(file, std::ios::in | std::ios::binary);

	if(!stream) {
		throw std::runtime_error("Could not open file for MD5, " + file);
	}

	auto remaining = std::filesystem::file_size(file);
	std::array<std::uint8_t, 16> res;
	auto hasher = Botan::HashFunction::create_or_throw("MD5");
	BOOST_ASSERT_MSG(hasher->output_length() == result.size(), "Bad hash size");
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

	hasher->final(res.data());
	return res;
}

} // util, ember