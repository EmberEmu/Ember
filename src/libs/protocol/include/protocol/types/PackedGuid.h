/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <bit>
#include <climits>
#include <cstdint>

namespace ember::protocol {

using Guid = std::uint64_t; // temp

struct PackedGuid final {
	Guid guid = 0;

	PackedGuid() = default;

	PackedGuid(Guid value)
		: guid(value) {}

	auto& operator<<(auto& stream) const {
		// produce the mask
		std::uint8_t mask = 0;
		std::uint8_t set = 0;

		for(auto i = 0; i < sizeof(std::uint64_t); ++i) {
			const auto seg_mask = static_cast<std::uint64_t>(0xff) << (i * CHAR_BIT);

			if(guid & seg_mask) {
				mask |= (1 << i);
				++set;
			}
		}

		stream << mask;

		if(set == CHAR_BIT) [[unlikely]] {
			stream << guid;
			return stream;
		}

		// send the set bytes
		std::uint8_t sent = 0;

		for(auto i = 0; i < sizeof(std::uint64_t); ++i) {
			const auto byte = static_cast<std::uint8_t>((guid >> (i * CHAR_BIT)) & 0xff);

			if(!byte) {
				continue;
			}

			stream << byte;
			
			if(++sent == set) {
				break;
			}
		}

		return stream;
	}

	auto& operator>>(auto& stream) {
		guid = 0; // clear current value
		std::uint8_t mask;
		stream >> mask;

		const auto count = std::popcount(mask);

		if(count == CHAR_BIT) [[unlikely]] {
			stream >> guid;
			return stream;
		}

		std::uint8_t bits_set = 0;

		for(auto i = 0; i < CHAR_BIT; ++i) {
			if(mask & (1 << i)) {
				std::uint8_t byte;
				stream >> byte;
				guid |= (static_cast<std::uint64_t>(byte) << (i * CHAR_BIT));

				if(++bits_set == count) {
					break;
				}
			}
		}

		return stream;
	}

	bool operator==(const PackedGuid& other) const {
		return guid == other.guid;
	}
};

} // protocol, ember