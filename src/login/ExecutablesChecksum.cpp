/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ExecutablesChecksum.h"
#include <fstream>
#include <botan/hmac.h>
#include <botan/sha160.h>
#include <memory>
#include <utility>
#include <cstddef>

namespace ember {

ExecutableChecksum::ExecutableChecksum(const std::string& path, std::initializer_list<std::string> files) {
	for(auto& f : files) {
		std::ifstream file(path + f, std::ios::binary | std::ios::ate);

		if(!file.is_open()) {
			throw std::runtime_error("Unable to open " + path + f);
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(static_cast<std::size_t>(size));

		if(!file.read(buffer.data(), size)) {
			throw std::runtime_error("Unable to read " + path + f);
		}

		buffers_.emplace_back(std::move(buffer));
	}
}

Botan::SecureVector<Botan::byte> ExecutableChecksum::checksum(const Botan::SecureVector<Botan::byte>& seed) const {
	auto sha160 = std::make_unique<Botan::SHA_160>();
	Botan::HMAC hmac(sha160.get()); // Botan takes ownership
	sha160.release(); // ctor didn't throw, relinquish the memory to Botan

	hmac.set_key(seed);

	for(auto& buffer : buffers_) {
		hmac.update(reinterpret_cast<const Botan::byte*>(buffer.data()), buffer.size());
	}

	return hmac.final();
}

Botan::SecureVector<Botan::byte> ExecutableChecksum::finalise(const Botan::SecureVector<Botan::byte>& checksum,
															  const std::uint8_t* client_seed, std::size_t len) {
	Botan::SHA_160 hasher;
	hasher.update(client_seed, len);
	hasher.update(checksum);
	auto hash = hasher.final();
	return hash;
}

} // ember