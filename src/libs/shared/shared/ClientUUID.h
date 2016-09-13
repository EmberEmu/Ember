/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/xoroshiro128plus.h>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace ember {

class ClientUUID {
	ClientUUID() : hash_(0) { }

	std::size_t hash_;

	union {
		std::uint8_t data_[16];

		struct {
			std::uint8_t service_;
			std::uint8_t rand_[15];
		};
	};

	std::size_t fnv_hash() {
		std::size_t hash = 0x811C9DC5;
		
		for(std::size_t i = 0; i < sizeof(data_); ++i) {
			hash = (hash * 0x1000193) ^ data_[i];
		}

		return hash;
	}

public:
	inline std::size_t hash() const {
		return hash_;
	}

	inline std::uint8_t service() const {
		return service_;
	}

	static ClientUUID from_bytes(const std::uint8_t* data) {
		ClientUUID uuid;
		std::copy(data, data + 16, uuid.data_);
		uuid.hash_ = uuid.fnv_hash();
		return uuid;
	}

	static ClientUUID generate(std::size_t service_index) {
		ClientUUID uuid;

		for(std::size_t i = 0; i < sizeof(data_); ++i) {
			uuid.data_[i] = static_cast<std::uint8_t>(rng::xorshift::next());
		}

		uuid.service_ = static_cast<std::uint8_t>(service_index); // todo
		uuid.hash_ = uuid.fnv_hash();
		return uuid;
	}

	friend bool operator==(const ClientUUID& rhs, const ClientUUID& lhs);
};

inline bool operator==(const ClientUUID& rhs, const ClientUUID& lhs) {
	return std::memcmp(rhs.data_, lhs.data_, sizeof(ClientUUID::data_)) == 0;
}

} // ember

template <>
struct std::hash<ember::ClientUUID> {
	std::size_t operator()(const ember::ClientUUID& uuid) const {
		return uuid.hash();
	}
};