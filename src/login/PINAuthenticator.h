/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/Clock.h>
#include <shared/utility/HashDefines.h>
#include <shared/utility/polyfill/inplace_vector>
#include <array>
#include <span>
#include <string_view>
#include <cstdint>

namespace ember {

class PINAuthenticator final {
public:
	static constexpr auto min_pin_length =  4u;
	static constexpr auto max_pin_length = 10u;
	static constexpr auto grid_size      = 10u;
	static constexpr auto key_length     = 10u;
	static constexpr auto salt_length    = 16u;
	
	using SaltBytes = std::array<std::uint8_t, salt_length>;
	using HashBytes = std::array<std::uint8_t, hash_sizes::sha160>;

private:
	std::array<std::uint8_t, grid_size> remapped_grid;
	std::inplace_vector<std::uint8_t, max_pin_length> pin_bytes_;

	void pin_to_ascii();
	void remap_pin_grid(std::uint32_t seed);
	void pin_to_bytes(std::uint32_t pin);
	void remap_pin();

public:
	explicit PINAuthenticator(std::uint32_t seed);

	HashBytes calculate_hash(const SaltBytes& server_salt,
	                         const SaltBytes& client_salt,
	                         std::uint32_t pin);

	bool validate_pin(const SaltBytes& server_salt,
	                  const SaltBytes& client_salt,
	                  std::span<const std::uint8_t> hash,
	                  std::uint32_t pin);

	static SaltBytes generate_salt();
	static std::uint32_t generate_seed();
	static std::uint32_t generate_totp_pin(const std::string_view secret,
	                                       int interval,
	                                       const utility::ClockBase& clock = utility::Clock());
};

} // ember