/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace ember {

class PacketCrypto {
	char key_[40];
	std::uint8_t _send_i = 0;
	std::uint8_t _send_j = 0;
	std::uint8_t _recv_i = 0;
	std::uint8_t _recv_j = 0;

public:
	void set_key(const char* key) {
		memcpy(key_, key, 40);
	}

	void encrypt(char* data, std::size_t length) {
		for(std::size_t t = 0; t < length; t++) {
			_send_i %= 40;
			std::uint8_t x = (data[t] ^ key_[_send_i]) + _send_j;
			++_send_i;
			data[t] = _send_j = x;
		}
	}

	void decrypt(char* data, std::size_t length) {
		for(std::size_t t = 0; t < length; t++) {
			_recv_i %= 40;
			std::uint8_t x = (data[t] - _recv_j) ^ key_[_recv_i];
			++_recv_i;
			_recv_j = data[t];
			data[t] = x;
		}
	}
};

} // ember