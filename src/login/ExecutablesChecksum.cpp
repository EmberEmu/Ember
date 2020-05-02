/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ExecutablesChecksum.h"
#include <botan/hash.h>
#include <botan/hmac.h>
#include <memory>

namespace ember::client_integrity {

Botan::secure_vector<std::uint8_t> checksum(const Botan::secure_vector<std::uint8_t>& seed,
                                            const std::vector<std::byte>* buffer) {
	auto sha160 = Botan::HashFunction::create_or_throw("SHA-1");
	Botan::HMAC hmac(sha160.get()); // Botan takes ownership
	sha160.release(); // ctor didn't throw, relinquish the memory to Botan

	hmac.set_key(seed);
	hmac.update(reinterpret_cast<const std::uint8_t*>(buffer->data()), buffer->size());
	return hmac.final();
}

Botan::secure_vector<std::uint8_t> finalise(const Botan::secure_vector<std::uint8_t>& checksum,
                                            const std::uint8_t* client_seed, std::size_t len) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	hasher->update(client_seed, len);
	hasher->update(checksum);
	return hasher->final();
}

} // client_integrity, ember