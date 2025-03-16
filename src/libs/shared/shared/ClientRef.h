/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/FNVHash.h>
#include <shared/utility/xoroshiro128plus.h>
#include <boost/functional/hash.hpp>
#include <gsl/narrow>
#include <algorithm>
#include <array>
#include <span>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace ember {

class ClientRef final {
	static constexpr std::size_t UUID_SIZE = 16;
	static constexpr std::size_t SERVICE_INDEX = 0;

	std::array<std::uint64_t, UUID_SIZE / sizeof(std::uint64_t)> data_;

	void generate(const std::size_t service_index) {
		for(auto& val : data_) {
			val = rng::xorshift::next();
		}

		auto bytes = std::as_writable_bytes(std::span(data_));
		bytes[SERVICE_INDEX] = gsl::narrow<std::byte>(service_index);
	}

public:
	explicit ClientRef(std::size_t service_index) {
		generate(service_index);
	}

	explicit ClientRef(std::span<const std::uint8_t, UUID_SIZE> data) {
		std::ranges::copy(data, data_.data());
	}

	inline std::size_t hash() const {
		FNVHash hasher;
		auto bytes = std::as_bytes(std::span(data_));

		for(auto byte : bytes) {
			hasher.update_byte(byte);
		}

		return hasher.finalise();
	}

	inline std::uint8_t service() const {
		return data_[SERVICE_INDEX];
	}

	// don't really care about efficiency here, it's for debugging
	inline std::string to_string() const {
		std::stringstream stream;
		stream << std::hex;

		auto bytes = std::as_bytes(std::span(data_));

		for(auto byte : bytes) {
			stream << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
		}

		return stream.str();
	}

	static constexpr auto size() {
		return UUID_SIZE;
	}

	friend bool operator==(const ClientRef& rhs, const ClientRef& lhs);
};

inline bool operator==(const ClientRef& rhs, const ClientRef& lhs) {
	return rhs.hash() == lhs.hash();
}

inline std::size_t hash_value(const ClientRef& uuid) {
	return uuid.hash();
}

} // ember

template<>
struct std::hash<ember::ClientRef> {
	std::size_t operator()(const ember::ClientRef& uuid) const {
		return uuid.hash();
	}
};
