/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <streambuf>

namespace ember::log::detail {

template<typename T>
struct StreamBuffer : std::streambuf {
	T& buffer;

	StreamBuffer(T& buf, std::size_t reserve_size = 0)
		: buffer(buf) {
		if(reserve_size > 0) {
			buffer.reserve(reserve_size);
		}
	}

protected:
	int_type overflow(int_type ch) override {
		if(ch != traits_type::eof()) {
			buffer.push_back(static_cast<char>(ch));
		}

		return ch;
	}

	std::streamsize xsputn(const char* s, std::streamsize n) override {
		buffer.insert(buffer.end(), s, s + n);
		return n;
	}
};

}