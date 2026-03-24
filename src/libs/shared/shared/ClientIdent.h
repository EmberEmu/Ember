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
#include <iomanip>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cstdint>

namespace ember {

class ClientIdent final {
	static constexpr std::size_t uuid_size = 16;
	static constexpr std::size_t service_offset = 0;

	std::array<std::uint64_t, uuid_size / sizeof(std::uint64_t)> data_;

	void generate(const std::size_t service_index) {
		for(auto& val : data_) {
			val = rng::xorshift::next();
		}

		auto bytes = std::as_writable_bytes(std::span(data_));
		bytes[service_offset] = gsl::narrow<std::byte>(service_index);
	}

public:
	explicit ClientIdent(std::size_t service_index) {
		generate(service_index);
	}

	explicit ClientIdent(std::span<const std::uint8_t, uuid_size> data) {
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
		return data_[service_offset];
	}

	// don't really care about efficiency here, it's for debugging
	inline std::string to_string() const {
		std::stringstream stream;
		stream << std::hex << std::setfill('0');

		const auto bytes = std::as_bytes(std::span(data_));

		for(auto byte : bytes) {
			stream << std::setw(2) << static_cast<int>(byte);
		}

		return stream.str();
	}

	static constexpr auto size() {
		return uuid_size;
	}

	friend bool operator==(const ClientIdent& rhs, const ClientIdent& lhs);
};

inline bool operator==(const ClientIdent& rhs, const ClientIdent& lhs) {
	return rhs.hash() == lhs.hash();
}

inline std::size_t hash_value(const ClientIdent& uuid) {
	return uuid.hash();
}

} // ember

template<>
struct std::hash<ember::ClientIdent> {
	std::size_t operator()(const ember::ClientIdent& uuid) const {
		return uuid.hash();
	}
};
