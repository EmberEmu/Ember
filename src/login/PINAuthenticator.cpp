/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PINAuthenticator.h"
#include <shared/util/xoroshiro128plus.h>
#include <shared/util/base32.h>
#include <boost/endian/conversion.hpp>
#include <botan/hmac.h>
#include <botan/sha160.h>
#include <gsl/gsl_util>
#include <memory>
#include <utility>
#include <cstddef>
#include <cmath>
#include <ctime>

namespace ember {

namespace be = boost::endian;

void PINAuthenticator::set_pin(std::uint64_t pin) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pin_to_bytes(pin); // convert to byte array
	remap_pin();       // calculate the expected input sequence
}

/*
 * Random number used by the client to 'randomise' the numpad layout.
 * We use this later on to remap our input grid to match that of the client.
 */
std::uint32_t PINAuthenticator::grid_seed() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	grid_seed_ = gsl::narrow_cast<std::uint32_t>(rng::xorshift::next());
	return grid_seed_;
}

/* 
 * Returns a completely random 16-byte salt used during hashing
 */
auto PINAuthenticator::server_salt() -> std::array<std::uint8_t, SALT_LENGTH>& {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	for(std::size_t i = 0; i < server_salt_.size(); ++i) {
		server_salt_[i] = gsl::narrow_cast<std::uint8_t>(rng::xorshift::next());
	}
	
	remap_pin_grid();
	return server_salt_;
}

/*
 * Converts a PIN such as '16785' into an array of bytes
 * {1, 6, 7, 8, 5} used during the hashing process.
 */
void PINAuthenticator::pin_to_bytes(std::uint64_t pin) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pin_bytes_.clear();

	while(pin != 0) {
		pin_bytes_.push_back(pin % 10);
		pin /= 10;
	}
	
	if(pin_bytes_.size() < MIN_PIN_LENGTH || pin_bytes_.size() > MAX_PIN_LENGTH) {
		throw std::invalid_argument("Incorrect PIN length provided");
	}

	std::reverse(pin_bytes_.begin(), pin_bytes_.end());
}

/* 
 * The client uses the grid seed to remap the numpad layout.
 * The server must use the seed to generate the same layout as the
 * client in order to calculate the expected input sequence from the
 * client. For example, if the user's PIN is '123' and the pad layout is
 * '0, 4, 1, 6, 2, 3' then the expected input sequence becomes '245'.
 */
void PINAuthenticator::remap_pin_grid() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	std::array<std::uint8_t, GRID_SIZE> grid { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	std::uint8_t* remapped_index = remapped_grid.data();
	std::uint32_t grid_seed = grid_seed_;

	for(std::size_t i = grid.size(); i > 0; --i) {
		auto remainder = grid_seed % i;
		grid_seed /= i;
		*remapped_index = grid[remainder];

		std::size_t copy_size = i;
		copy_size -= remainder;
		--copy_size;

		std::uint8_t* src_ptr = grid.data() + remainder + 1;
		std::uint8_t* dst_ptr = grid.data() + remainder;

		std::copy(src_ptr, src_ptr + copy_size, dst_ptr);
		++remapped_index;
	}
}

/* 
 * Takes the user's PIN and the remapped grid to figure out the expected
 * input sequence. That is, calculate the indices of the buttons that the user
 * will press on the game's numpad.
 */
void PINAuthenticator::remap_pin() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	for(std::size_t i = 0; i < pin_bytes_.size(); ++i) {
		auto index = std::find(remapped_grid.begin(), remapped_grid.end(), pin_bytes_[i]);
		pin_bytes_[i] = std::distance(remapped_grid.begin(), index);
	}
}

/*
 * Converts the PIN bytes into ASCII values by simply adding 0x30.
 * Mutates the original bytes rather than creating a copy for efficiency.
 * Why bother doing this? Security, or something. I didn't invent this algorithm.
 */
void PINAuthenticator::pin_to_ascii() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	for(std::size_t i = 0; i < pin_bytes_.size(); ++i) {
		pin_bytes_[i] += 0x30;
	}
}

bool PINAuthenticator::validate_pin(const std::array<std::uint8_t, HASH_LENGTH>& client_hash) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pin_to_ascii();

	// x = H(client_salt | H(server_salt | ascii(pin_bytes)))
	Botan::SHA_160 hasher;
	hasher.update(server_salt_.data(), server_salt_.size());
	hasher.update(pin_bytes_.data(), pin_bytes_.size());
	auto hash = hasher.final();

	hasher.update(client_salt_.data(), client_salt_.size());
	hasher.update(hash);
	auto final_hash = hasher.final();

	// ensure we computed the same result as the client
	return std::equal(final_hash.begin(), final_hash.end(), client_hash.begin(), client_hash.end());
}

std::uint32_t PINAuthenticator::generate_totp_pin(const std::string& secret, int interval) {
	std::vector<std::uint8_t> decoded_key((secret.size() + 7) / 8 * 5);
	int key_size = base32_decode((const uint8_t*)secret.data(), decoded_key.data(), decoded_key.size());

	if(key_size == -1) {
		throw std::invalid_argument("Unable to base32 decode TOTP key, " + secret);
	}

	// not guaranteed by the standard to be the UNIX epoch but it is on all supported platforms
	auto time = std::time(NULL);
	std::uint64_t now = static_cast<std::uint64_t>(time);
	std::uint64_t step = static_cast<std::uint64_t>((std::floor(now / 30))) + interval;

	auto sha160 = std::make_unique<Botan::SHA_160>();
	Botan::HMAC hmac(sha160.get()); // Botan takes ownership
	sha160.release(); // ctor didn't throw, relinquish the memory to Botan

	hmac.set_key(decoded_key.data(), key_size);

#if defined(BOOST_LITTLE_ENDIAN) 
	hmac.update_be(step);
#else
	hmac.update(step);
#endif

	auto hmac_result = hmac.final();

	unsigned int offset = hmac_result[19] & 0xF;
	std::uint32_t pin = (hmac_result[offset] & 0x7f) << 24 | (hmac_result[offset + 1] & 0xff) << 16
	                     | (hmac_result[offset + 2] & 0xff) << 8 | (hmac_result[offset + 3] & 0xff);

    boost::endian::little_to_native_inplace(pin);

	pin &= 0x7FFFFFFF;
	pin %= 1000000;
	return pin;
}

} // ember