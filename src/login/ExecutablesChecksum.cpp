/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ExecutablesChecksum.h"
#include <botan/hmac.h>
#include <botan/sha160.h>
#include <memory>

namespace ember { namespace client_integrity {

Botan::secure_vector<Botan::byte> checksum(const Botan::secure_vector<Botan::byte>& seed,
                                          const std::vector<char>* buffer) {
	auto sha160 = std::make_unique<Botan::SHA_160>();
	Botan::HMAC hmac(sha160.get()); // Botan takes ownership
	sha160.release(); // ctor didn't throw, relinquish the memory to Botan

	hmac.set_key(seed);
	hmac.update(buffer->data());
	return hmac.final();
}

Botan::secure_vector<Botan::byte> finalise(const Botan::secure_vector<Botan::byte>& checksum,
                                          const std::uint8_t* client_seed, std::size_t len) {
	Botan::SHA_160 hasher;
	hasher.update(client_seed, len);
	hasher.update(checksum);
	return hasher.final();
}

}} // client_integrity, ember