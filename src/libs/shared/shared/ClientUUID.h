/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/FNVHash.h>
#include <shared/util/xoroshiro128plus.h>
#include <boost/functional/hash.hpp>
#include <gsl/gsl_util>
#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace ember {

class ClientUUID {
	mutable std::size_t hash_ = 0;
	mutable bool hashed_ = false;

	union {
		std::uint8_t data_[16];

		struct {
			std::uint8_t service_;
			std::uint8_t rand_[15];
		};
	};

public:
	inline std::size_t hash() const {
		if(!hashed_) {
			FNVHash hasher;
			hash_ = hasher.update(std::begin(data_), std::end(data_));
			hashed_ = true;
		}

		return hash_;
	}

	inline std::uint8_t service() const {
		return service_;
	}

	static ClientUUID from_bytes(const std::array<std::uint8_t, 16>& data) {
		ClientUUID uuid;
		std::copy(data.begin(), data.end(), uuid.data_);
		return uuid;
	}

	static ClientUUID generate(std::size_t service_index) {
		ClientUUID uuid;

		for(std::size_t i = 0; i < sizeof(data_); ++i) {
			uuid.data_[i] = gsl::narrow_cast<std::uint8_t>(rng::xorshift::next());
		}

		uuid.service_ = gsl::narrow<std::uint8_t>(service_index);
		return uuid;
	}

	friend bool operator==(const ClientUUID& rhs, const ClientUUID& lhs);
};

inline bool operator==(const ClientUUID& rhs, const ClientUUID& lhs) {
	return rhs.hash() == lhs.hash();
}

} // ember

template <>
struct std::hash<ember::ClientUUID> {
	std::size_t operator()(const ember::ClientUUID& uuid) const {
		return uuid.hash();
	}
};