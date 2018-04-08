/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <gsl/span>
#include <string>
#include <type_traits>
#include <cstddef>

namespace ember {

class FNVHash {
	static constexpr std::size_t INITIAL = 0x811C9DC5;
	std::size_t hash_ = INITIAL;

public:
	template<typename ItBeg, typename ItEnd>
	std::size_t update(ItBeg begin, ItEnd end) {
		
		for(auto it = begin; begin != end; ++begin) {
			hash_ = (hash_ * 0x1000193) ^ static_cast<char>(*it);
		}

		return hash_;
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, std::size_t>::type update(T data) {
		const auto span = gsl::make_span(&data, sizeof(T));
		return update(span.begin(), span.end());
	}

	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, std::size_t>::type update(T data) {
		return update(static_cast<std::underlying_type<T>::type>(data));
	}

	template<typename T>
	std::size_t update(const T* data, std::size_t len) {
		const auto span = gsl::make_span(&data, sizeof(T));
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