/*
 * Copyright (c) 2016 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/Clock.h>
#include <logger/Logging.h>
#include <boost/container/static_vector.hpp>
#include <array>
#include <span>
#include <string>
#include <cstdint>

namespace ember {

class PINAuthenticator final {
public:
	static constexpr int MIN_PIN_LENGTH =  4;
	static constexpr int MAX_PIN_LENGTH = 10;
	static constexpr int GRID_SIZE      = 10;
	static constexpr int KEY_LENGTH     = 10;
	static constexpr int SALT_LENGTH    = 16;
	static constexpr int HASH_LENGTH    = 20;
	
	using SaltBytes = std::array<std::uint8_t, SALT_LENGTH>;
	using HashBytes = std::array<std::uint8_t, HASH_LENGTH>;

private:
	log::Logger* logger_;
	const std::uint32_t grid_seed_;
	SaltBytes server_salt_;
	SaltBytes client_salt_;
	std::array<std::uint8_t, GRID_SIZE> remapped_grid;
	boost::container::static_vector<std::uint8_t, MAX_PIN_LENGTH> pin_bytes_;

	void pin_to_ascii();
	void remap_pin_grid();
	void pin_to_bytes(std::uint32_t pin);
	void remap_pin();

public:
	PINAuthenticator(const SaltBytes& server_salt, const SaltBytes& client_salt,
	                 std::uint32_t seed, log::Logger* logger);

	HashBytes calculate_hash(const std::uint32_t pin);
	bool validate_pin(std::uint32_t pin, std::span<const std::uint8_t> hash);

	static SaltBytes generate_salt();
	static std::uint32_t generate_seed();
	static std::uint32_t generate_totp_pin(const std::string& secret, int interval,
	                                       const util::ClockBase& clock = util::Clock());
};

} // ember