/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/IPBanBase.h>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v6_range.hpp>
#include <stdexcept>
#include <span>
#include <vector>
#include <cstdint>

namespace ember {

class IPBanCache {
	struct IPv4Entry {
		boost::asio::ip::address_v4::uint_type range;
		std::uint32_t mask;
	};

	std::vector<IPv4Entry> ipv4_entries_;
	std::vector<boost::asio::ip::address_v6_range> ipv6_entries_;

	// https://stackoverflow.com/a/57288759 because lazy
	boost::asio::ip::address_v6_range build_range(const boost::asio::ip::address_v6& address,
	                                              const std::uint32_t prefix) const {
		auto bytes = address.to_bytes();
		auto offset = prefix >> 3;
		auto shift = 1 << (8 - (prefix & 0x07));

		while(shift) {
			const auto value = bytes[offset] + shift;
			bytes[offset] = value & 0xFF;
			shift = value >> 8;

			if(offset == 0) {
				break;
			}

			--offset;
		}

		const auto end = boost::asio::ip::address_v6(bytes);
		return boost::asio::ip::address_v6_range(address, end);
	}

	bool check_ban(const boost::asio::ip::address_v6&& ip) const {
		for(auto& range : ipv6_entries_) {
			if(auto it = range.find(ip); it != range.end()) {
				return true;
			}
		}

		return false;
	}

	bool check_ban(const boost::asio::ip::address_v4&& ip) const {
		unsigned long ip_long = ip.to_ulong();
		
		for(auto& e : ipv4_entries_) {
			if((ip_long & e.mask) == (e.range & e.mask)) {
				return true;
			}
		}

		return false;
	}

	void load_bans(std::span<const IPEntry> bans) {
		for(auto& [ip, cidr] : bans) {
			load_ban(ip, cidr);
		}
	}

	void load_ban(const std::string& ip, const std::uint32_t cidr) {
		auto address = boost::asio::ip::address::from_string(ip);

		if(address.is_v6()) {
			auto range = build_range(address.to_v6(), cidr);
			ipv6_entries_.emplace_back(std::move(range));
		} else {
			std::uint32_t mask = (~0U) << (32 - cidr);
			ipv4_entries_.emplace_back(address.to_v4().to_uint(), mask);
		}
	}

public:
	IPBanCache(std::span<const IPEntry> bans) {
		load_bans(bans);
	}

	IPBanCache() = default;

	void ban(const std::string& ip, const std::uint32_t mask) {
		load_ban(ip, mask);
	}

	bool is_banned(const std::string& ip) const {
		if(ipv4_entries_.empty() && ipv6_entries_.empty()) {
			return false;
		}

		return is_banned(boost::asio::ip::address::from_string(ip));
	}

	bool is_banned(const boost::asio::ip::address& ip) const {
		if(ip.is_v6()) {
			return check_ban(ip.to_v6());
		} else {
			return check_ban(ip.to_v4());
		}
	}
};

} // ember