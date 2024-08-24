/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ExecutablesChecksum.h"
#include <boost/assert.hpp>
#include <botan/sha1/sha1.h>
#include <botan/mac.h>

namespace ember::client_integrity {

std::array<std::uint8_t, 20> checksum(std::span<const std::uint8_t> seed,
                                      std::span<const std::byte> buffer) {
	std::array<std::uint8_t, 20> res;
	auto hmac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-1)");
	BOOST_ASSERT_MSG(hmac->output_length() == res.size(), "Bad hash size");
	hmac->set_key(seed.data(), seed.size());
	hmac->update(reinterpret_cast<const std::uint8_t*>(buffer.data()), buffer.size_bytes());
	hmac->final(res.data());
	return res;
}

std::array<std::uint8_t, 20> finalise(std::span<const std::uint8_t> checksum,
                                      std::span<const std::uint8_t> client_seed) {
	std::array<std::uint8_t, 20> res;
	Botan::SHA_1 hasher;
	BOOST_ASSERT_MSG(hasher.output_length() == res.size(), "Bad hash size");
	hasher.update(client_seed.data(), client_seed.size_bytes());
	hasher.update(checksum.data(), checksum.size_bytes());
	hasher.final(res.data());
	return res;
}

} // client_integrity, ember