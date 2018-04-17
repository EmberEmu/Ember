/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Buffer.h>
#include <botan/types.h>
#include <boost/assert.hpp>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace ember {

class PacketCrypto final {
	std::vector<Botan::byte> key_;
	std::uint8_t send_i_ = 0;
	std::uint8_t send_j_ = 0;
	std::uint8_t recv_i_ = 0;
	std::uint8_t recv_j_ = 0;

public:
	PacketCrypto() = default;
	explicit PacketCrypto(std::vector<Botan::byte> key) : key_(std::move(key)) {}

	void set_key(std::vector<Botan::byte> key) {
		key_ = std::move(key);
	}

	template<typename T>
	void encrypt(T& data) {
		BOOST_ASSERT_MSG(!key_.empty(), "Session key empty when encrypting");

		auto d_bytes = reinterpret_cast<std::uint8_t*>(&data);

		for(std::size_t t = 0; t < sizeof(T); ++t) {
			send_i_ %= key_.size();
			std::uint8_t x = (d_bytes[t] ^ key_[send_i_]) + send_j_;
			++send_i_;
			d_bytes[t] = send_j_ = x;
		}
	}

	void encrypt(spark::Buffer& data, std::size_t length) {
		for(std::size_t t = 0; t < length; ++t) {
			send_i_ %= key_.size();
			auto& byte = reinterpret_cast<char&>(data[t]);
			std::uint8_t x = (byte ^ key_[send_i_]) + send_j_;
			++send_i_;
			byte = send_j_ = x;
		}
	}

	void decrypt(spark::Buffer& data, std::size_t length) {
		BOOST_ASSERT_MSG(!key_.empty(), "Session key empty when decrypting");

		for(std::size_t t = 0; t < length; ++t) {
			recv_i_ %= key_.size();
			auto& byte = reinterpret_cast<char&>(data[t]);
			std::uint8_t x = (byte - recv_j_) ^ key_[recv_i_];
			++recv_i_;
			recv_j_ = byte;
			byte = x;
		}
	}
};

} // ember