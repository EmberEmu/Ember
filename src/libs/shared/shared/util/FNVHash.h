/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <type_traits>
#include <cstddef>

namespace ember {

class FNVHash {
	static const std::size_t INITIAL = 0x811C9DC5;
	std::size_t hash_ = INITIAL;

public:
	template<typename T>
	std::size_t update(const T& data) {
		const char* data_ = reinterpret_cast<const char*>(&data);

		for(std::size_t i = 0; i < sizeof(data); ++i) {
			hash_ = (hash_ * 0x1000193) ^ data_[i];
		}

		return hash_;
	}

	template<typename T>
	std::size_t update(const T* data, std::size_t len) {
		for(std::size_t i = 0; i < len; ++i) {
			hash_ = (hash_ * 0x1000193) ^ data[i];
		}

		return hash_;
	}

	std::size_t update(const std::string& data) {
		for(std::size_t i = 0; i < data.size(); ++i) {
			hash_ = (hash_ * 0x1000193) ^ data[i];
		}

		return hash_;
	}

	void reset() {
		hash_ = INITIAL;
	}

	std::size_t hash() const {
		return hash_;
	}

	std::size_t finalise() {
		std::size_t ret = hash_;
		hash_ = INITIAL;
		return ret;
	}
};

}