/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/util/FileMD5.h>
#include <botan/md5.h>
#include <botan/bigint.h>
#include <vector>
#include <fstream>

namespace ember { namespace util {

Botan::BigInt generate_md5(const std::string& file) {
	std::fstream stream(file, std::ios::in | std::ios::binary);

	if(!stream.is_open()) {
		return "";
	}

	stream.seekg(0, std::ios::end);
	std::size_t remaining = stream.tellg();
	stream.seekg(0, std::ios::beg);

	Botan::MD5 hasher;
	std::size_t block_size = hasher.hash_block_size();
	std::vector<char> buffer(block_size);

	while(remaining) {
		std::size_t read_size = remaining > block_size? block_size : remaining;
		stream.read(buffer.data(), read_size);
		hasher.update(reinterpret_cast<Botan::byte*>(buffer.data()), read_size);
		remaining -= read_size;

		if(!stream.good()) {
			return "";
		}
	}

	return Botan::BigInt::decode(hasher.final());
}

}} // util, ember