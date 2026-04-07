/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/FNVHash.h>
#include <shared/utility/xoroshiro128plus.h>
#include <gsl/narrow>
#include <algorithm>
#include <array>
#include <iomanip>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ember {

/*
 * This is used primarily to allow remote services to identify particular clients
 * over the network. The realm gateway is the central authority for a cluster that
 * generates and maintains identities. If a remote service wishes to communicate
 * with a particular client/game session, it must do so using its ClientIdent.
 * 
 * To all services except the realm's dispatcher, the ident should be opaque -
 * which is to say that it's just a bunch of random bytes and nothing more.
 * 
 * Internally, the dispatcher encodes several pieces of information within the
 * identity; the client's service index (which thread services it), an encoded
 * pointer to the client's message handler, a lookup index, and random bytes.
 * 
 * This encoding allows the dispatcher to efficiently check whether the referenced
 * client/game session still exists and ensure that the message is dispatched
 * to the correct thread, with no need for a central registry or locking.
 * 
 * In the unlikely event that the pointer cannot be encoded on a particular
 * architecture, the dispatcher can fall back to using an associative map via a
 * compile-time flag. There are still no locks but dispatching won't be as fast.
 */
class ClientIdent final {
	static constexpr std::size_t uuid_size = 16;

	std::array<std::uint64_t, uuid_size / sizeof(std::uint64_t)> data_;
	mutable std::size_t hash_;
	mutable bool hashed_;

	void generate(const std::size_t service_index) {
		assert(service_index < 256);

		for(auto& val : data_) {
			val = rng::xorshift::next();
		}

		encode_index(service_index);
	}

	void encode_index(const std::uint8_t index) {
		data_[0] &= 0x00ffffffffffffffull;
		data_[0] |= static_cast<std::uint64_t>(index) << 56;
	}

public:
	constexpr static auto max_slot_value = 0xfff;
	static_assert(max_slot_value < 0x1000, "Slot count exceeds bits assigned for encoding");

	ClientIdent()
		: data_{}
		, hash_(0)
		, hashed_(false) {};

	explicit ClientIdent(std::size_t service_index)
		: ClientIdent() {
		generate(service_index);
	}

	explicit ClientIdent(std::span<const std::uint8_t, uuid_size> data)
		: ClientIdent() {
		std::ranges::copy(data, data_.data());
	}

	// Encodes a pointer using 48 bits (widens on 32-bit platforms) and a 12-bit array index
	void encode(const void* ptr, std::size_t slot) {
		assert(slot <= max_slot_value);
		std::uint64_t ptr_val = reinterpret_cast<std::uint64_t>(ptr);
		ptr_val &= 0xffffffffffffull;
		const std::uint8_t slot_hi = (slot >> 4) & 0xff;
		const std::uint8_t slot_lo = slot & 0xf;
		data_[0] = (data_[0] & (0xffull << 56)) | (ptr_val << 8ull) | slot_hi;
		data_[1] = (data_[1] & ~0xfull) | slot_lo;
	}

	template<typename T>
	T* extract_ptr() const {
		return reinterpret_cast<T*>((data_[0] >> 8) & 0xffffffffffff);
	}

	std::size_t extract_slot() const {
		return ((data_[0] & 0xff) << 4) | (data_[1] & 0xf);
	}

	inline std::size_t hash() const {
		if(hashed_) {
			return hash_;
		}

		FNVHash hasher;
		auto bytes = std::as_bytes(std::span(data_));

		for(auto byte : bytes) {
			hasher.update_byte(byte);
		}

		hashed_ = true;
		return hash_ = hasher.finalise();
	}

	inline std::uint8_t service() const {
		return data_[0] >> 56;
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

	void set_zero() {
		data_[0] = 0;
		data_[1] = 0;
	}

	bool is_zero() const {
		return data_[0] == 0 && data_[1] == 0;
	}

	static consteval auto size() {
		return uuid_size;
	}

	friend bool operator==(const ClientIdent& lhs, const ClientIdent& rhs);
};

inline bool operator==(const ClientIdent& lhs, const ClientIdent& rhs) {
	return rhs.data_[0] == lhs.data_[0]
		&& lhs.data_[1] == lhs.data_[1];
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
