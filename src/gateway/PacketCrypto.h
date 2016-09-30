/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Buffer.h>
#include <botan/bigint.h>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace ember {

class PacketCrypto {
	Botan::secure_vector<Botan::byte> key_;
	std::uint8_t send_i_ = 0;
	std::uint8_t send_j_ = 0;
	std::uint8_t recv_i_ = 0;
	std::uint8_t recv_j_ = 0;

public:
	void set_key(const Botan::secure_vector<Botan::byte>& key) {
		key_ = key;
	}

	void encrypt(spark::Buffer& data, std::size_t offset, std::size_t length) {
		for(std::size_t t = 0; t < length; ++t) {
			send_i_ %= key_.size();
			char& byte = data[offset + t]; // todo - type change
			std::uint8_t x = (byte ^ key_[send_i_]) + send_j_;
			++send_i_;
			byte = send_j_ = x;
		}
	}

	void decrypt(spark::Buffer& data, std::size_t length) {
		for(std::size_t t = 0; t < length; ++t) {
			recv_i_ %= key_.size();
			char& byte = data[t]; // todo - type change
			std::uint8_t x = (byte - recv_j_) ^ key_[recv_i_];
			++recv_i_;
			recv_j_ = byte;
			byte = x;
		}
	}
};

} // ember