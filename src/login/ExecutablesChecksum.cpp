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
	Botan::HMAC hasher(new Botan::SHA_160()); // not a leak, Botan takes ownership
	hasher.set_key(seed);

	for(auto& buffer : buffers_) {
		hasher.update(reinterpret_cast<const Botan::byte*>(buffer.data()), buffer.size());
	}

	return hasher.final();
}

Botan::SecureVector<Botan::byte> ExecutableChecksum::finalise(const Botan::SecureVector<Botan::byte>& checksum,
                                                              const Botan::BigInt& client_seed) {
	auto encoded = Botan::BigInt::encode(client_seed);
	std::reverse(encoded.begin(), encoded.end());

	Botan::SHA_160 hasher;
	hasher.update(encoded);
	hasher.update(checksum);
	auto hash = hasher.final();
	return hash;
}

} // ember