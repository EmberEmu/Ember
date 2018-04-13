/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <array>
#include <cstdint>
#include <vector>

namespace ember {

class PINAuthenticator final {
	static constexpr int MIN_PIN_LENGTH =  4;
	static constexpr int MAX_PIN_LENGTH = 10;
	static constexpr int GRID_SIZE      = 10;
	static constexpr int SALT_LENGTH    = 16;
	static constexpr int HASH_LENGTH    = 20;
	
	std::uint64_t pin_;
	std::uint32_t grid_seed_;
	std::array<std::uint8_t, SALT_LENGTH> client_salt_;
	std::array<std::uint8_t, SALT_LENGTH> server_salt_;
	std::array<std::uint8_t, HASH_LENGTH> client_hash_;
	std::array<std::uint8_t, GRID_SIZE> remapped_grid;
	std::vector<std::uint8_t> pin_bytes_;

	log::Logger* logger_;

	void pin_to_ascii();
	void remap_pin_grid();
	void pin_to_bytes(std::uint64_t pin);
	void remap_pin();

public:
	explicit PINAuthenticator(log::Logger* logger) : logger_(logger), pin_(0), grid_seed_(0) {}

	const std::array<std::uint8_t, SALT_LENGTH>& server_salt();
	std::uint32_t grid_seed();
	void set_pin(std::uint64_t pin);
	bool validate_pin(const std::array<std::uint8_t, HASH_LENGTH>& hash);

	void set_client_hash(const std::array<std::uint8_t, HASH_LENGTH>& hash) {
		client_hash_ = hash;
	}

	void set_client_salt(const std::array<std::uint8_t, SALT_LENGTH>& salt) {
		client_salt_ = salt;
	}

	static std::uint32_t generate_totp_pin(const std::string& secret, int interval);
};

} // ember