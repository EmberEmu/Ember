/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <botan/bigint.h>
#include <boost/assert.hpp>
#include <boost/container/small_vector.hpp>
#include <limits>
#include <span>
#include <cstdint>
#include <cstddef>

namespace ember::realm {


/*
 * This uses key doubling to simplify the conditional checks that need to be performed
 * per byte or per call if using a non-zero templated key size. That gives it a very slight
 * performance edge across all tested compilers against the simpler version where the indices
 * cannot be allowed to exceed the size of the key passed to the constructor.
 * 
 * Additionally, modulo for bounds checking has been replaced with conditionals, as both 
 * GCC and msvc produced significantly slower code with it. Clang's output was already better
 * and didn't see much benefit.
 * 
 * The calls that take a reference for _key_size = 0 aren't implemented in terms of the
 * pointer/length overload because the generated code was very marginally slower.
 * 
 * Using the doubled key size in the non-templated conditionals to try to reduce the number of
 * index resets to zero yielded slightly slower code. GT & subtraction rather than GTE & zero
 * assignment made little difference. This is well into yak shaving territory, so it'll do.
 * 
 */
template<std::size_t _key_size = 0>
requires(_key_size < 256)
class PacketCrypto final {
	static constexpr auto key_size_hint = _key_size > 0? _key_size * 2 : 64;

	boost::container::small_vector<std::uint8_t, key_size_hint> key_;
	std::uint8_t halved_key_size = 0;
	std::uint8_t send_i_ = 0;
	std::uint8_t send_j_ = 0;
	std::uint8_t recv_i_ = 0;
	std::uint8_t recv_j_ = 0;

public:
	explicit PacketCrypto(std::span<const std::uint8_t> key) {
		if constexpr(_key_size) {
			if(key.size() != _key_size) {
				throw std::runtime_error("Session key did not match expected size");
			}
		} else if(key.size() > std::numeric_limits<std::uint8_t>::max()) {
			throw std::runtime_error("Session key too big");
		} else if(key.empty()) {
			throw std::runtime_error("Session key empty");
		}

		halved_key_size = static_cast<std::uint8_t>(key.size());
		key_.resize(key.size() * 2, boost::container::default_init);
		std::copy(key.begin(), key.end(), key_.begin());
		std::copy(key.begin(), key.end(), key_.begin() + halved_key_size);
	}


	inline void encrypt(auto& data) requires (_key_size > 0 && sizeof(data) <= _key_size) {
		auto data_bytes = reinterpret_cast<std::uint8_t*>(&data);

		for(std::size_t t = 0; t < sizeof(data); ++t) {
			std::uint8_t x = (data_bytes[t] ^ key_[send_i_]) + send_j_;
			++send_i_;
			data_bytes[t] = send_j_ = x;
		}

		if(send_i_ > _key_size) {
			send_i_ -= _key_size;
		}
	}

	inline void encrypt(auto& data) {
		auto data_bytes = reinterpret_cast<std::uint8_t*>(&data);

		for(std::size_t t = 0; t < sizeof(data); ++t) {
			if(send_i_ >= halved_key_size) {
				send_i_ = 0;
			}

			std::uint8_t x = (data_bytes[t] ^ key_[send_i_]) + send_j_;
			++send_i_;
			data_bytes[t] = send_j_ = x;
		}
	}

	inline void encrypt(auto* data, const std::size_t length) {
		auto data_bytes = reinterpret_cast<std::uint8_t*>(data);

		for(std::size_t i = 0; i < length; ++i) {
			if(send_i_ >= halved_key_size) {
				send_i_ = 0;
			}

			const std::uint8_t x = (data_bytes[i] ^ key_[send_i_]) + send_j_;
			data_bytes[i] = send_j_ = x;
			++send_i_;
		}
	}

	inline void decrypt(auto& data) requires (_key_size > 0 && sizeof(data) <= _key_size) {
		auto data_bytes = reinterpret_cast<std::uint8_t*>(&data);

		for(std::size_t t = 0; t < sizeof(data); ++t) {
			auto& byte = data_bytes[t];
			const std::uint8_t x = (byte - recv_j_) ^ key_[recv_i_];
			recv_j_ = byte;
			byte = x;
			++recv_i_;
		}

		if(recv_i_ > _key_size) {
			recv_i_ -= _key_size;
		}
	}

	inline void decrypt(auto& data) {
		auto data_bytes = reinterpret_cast<std::uint8_t*>(&data);

		for(std::size_t t = 0; t < sizeof(data); ++t) {
			if(recv_i_ >= halved_key_size) {
				recv_i_ = 0;
			}

			auto& byte = data_bytes[t];
			const std::uint8_t x = (byte - recv_j_) ^ key_[recv_i_];
			recv_j_ = byte;
			byte = x;
			++recv_i_;
		}
	}

	inline void decrypt(auto* data, const std::size_t length) {
		auto data_bytes = reinterpret_cast<std::uint8_t*>(data);

		for(std::size_t i = 0; i < length; ++i) {
			if(recv_i_ >= halved_key_size) {
				recv_i_ = 0;
			}

			auto& byte = data_bytes[i];
			const std::uint8_t x = (byte - recv_j_) ^ key_[recv_i_];
			++recv_i_;
			recv_j_ = byte;
			byte = x;
		}
	}
};

} // realm, ember