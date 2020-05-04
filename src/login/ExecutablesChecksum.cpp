/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ExecutablesChecksum.h"
#include <botan/hash.h>
#include <botan/mac.h>

namespace ember::client_integrity {

Botan::secure_vector<std::uint8_t> checksum(const Botan::secure_vector<std::uint8_t>& seed,
                                            const std::vector<std::byte>* buffer) {
	auto hmac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-1)");
	hmac->set_key(seed);
	hmac->update(reinterpret_cast<const std::uint8_t*>(buffer->data()), buffer->size());
	return hmac->final();
}

Botan::secure_vector<std::uint8_t> finalise(const Botan::secure_vector<std::uint8_t>& checksum,
                                            const std::span<uint8_t> client_seed) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	hasher->update(client_seed.data(), client_seed.size_bytes());
	hasher->update(checksum);
	return hasher->final();
}

} // client_integrity, ember