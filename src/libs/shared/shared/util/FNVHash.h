/*
 * Copyright (c) 2016 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <concepts>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <cstddef>

namespace ember {

template<typename T>
concept is_enum = std::is_enum<T>::value;

class FNVHash {
	static constexpr std::size_t INITIAL = 0x811C9DC5;
	std::size_t hash_ = INITIAL;

public:
	template<typename It>
	std::size_t update(It begin, const It end) {
		for(; begin != end; ++begin) {
			hash_ = (hash_ * 0x1000193) ^ static_cast<char>(*begin);
		}

		return hash_;
	}

	template<std::integral T>
	std::size_t update(T data) {
		const std::span<T> span(&data, sizeof(data));
		return update(span.begin(), span.end());
	}

	template<is_enum T>
	std::size_t update(T data) {
		return update(std::to_underlying(data));
	}

	template<typename T>
	std::size_t update(std::span<T> span) {
		return update(span.begin(), span.end());
	}

	std::size_t update(const std::string& t) {
		return update(t.begin(), t.end());
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
