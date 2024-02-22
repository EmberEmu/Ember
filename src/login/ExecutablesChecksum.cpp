/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ExecutablesChecksum.h"
#include <botan/hash.h>
#include <botan/mac.h>

namespace ember::client_integrity {

std::vector<std::uint8_t> checksum(const std::span<const std::uint8_t> seed,
                                   std::span<const std::byte> buffer) {
	auto hmac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-1)");
	hmac->set_key(seed.data(), seed.size());
	hmac->update(reinterpret_cast<const std::uint8_t*>(buffer.data()), buffer.size_bytes());
	return hmac->final_stdvec();
}

std::vector<std::uint8_t> finalise(std::span<const std::uint8_t> checksum,
                                   std::span<const std::uint8_t> client_seed) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	hasher->update(client_seed.data(), client_seed.size_bytes());
	hasher->update(checksum.data(), checksum.size_bytes());
	return hasher->final_stdvec();
}

} // client_integrity, ember