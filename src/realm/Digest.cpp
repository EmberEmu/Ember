/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Digest.h"
#include <boost/assert.hpp>
#include <botan/bigint.h>
#include <botan/hash.h>

namespace ember::realm::digest {

std::array<std::uint8_t, hash_sizes::sha160> calculate(std::span<const std::uint8_t> key,
                                                       const std::string_view username,
                                                       std::uint32_t protocol_id,
                                                       std::uint32_t client_seed,
                                                       std::uint32_t server_seed) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	std::array<std::uint8_t, hash_sizes::sha160> hash;
	BOOST_ASSERT_MSG(hash.size() == hasher->output_length(), "Bad hash length");
	hasher->update(username);
	hasher->update_le(protocol_id);
	hasher->update_le(client_seed);
	hasher->update_le(server_seed);
	hasher->update(key);
	hasher->final(hash);
	return hash;
}

} // digest, realm, ember