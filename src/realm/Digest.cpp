/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Digest.h"
#include <algorithm>
#include <boost/assert.hpp>
#include <botan/hash.h>

namespace ember::realm::digest {

bool validate(const Context& ctx, std::span<const std::uint8_t, hash_sizes::sha160> client_digest) {
	const auto result = calculate(ctx);
	return std::ranges::equal(result, client_digest);
}

std::array<std::uint8_t, hash_sizes::sha160> calculate(const Context& ctx) {
	auto hasher = Botan::HashFunction::create_or_throw("SHA-1");
	std::array<std::uint8_t, hash_sizes::sha160> hash;
	BOOST_ASSERT_MSG(hash.size() == hasher->output_length(), "Bad hash length");
	hasher->update(ctx.username);
	hasher->update_le(ctx.protocol_id);
	hasher->update_le(ctx.client_seed);
	hasher->update_le(ctx.server_seed);
	hasher->update(ctx.key);
	hasher->final(hash);
	return hash;
}

} // digest, realm, ember