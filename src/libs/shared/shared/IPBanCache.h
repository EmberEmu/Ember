/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/IPBanBase.h>
#include <boost/asio/ip/address.hpp>
#include <stdexcept>
#include <vector>
#include <cstdint>

namespace ember {

class IPBanCache {
	struct IPv4Entry {
		std::uint32_t range;
		std::uint32_t mask;
	};

	std::vector<IPv4Entry> entries_;

	bool check_ban(const boost::asio::ip::address_v6&& ip) {
		//implement
		return false;
	}

	bool check_ban(const boost::asio::ip::address_v4&& ip) {
		unsigned long ip_long = ip.to_ulong();
		
		for(auto& e : entries_) {
			if((ip_long & e.mask) == (e.range & e.mask)) {
				return true;
			}
		}

		return false;
	}

	void load_bans(const std::vector<IPEntry>& bans) {
		for(auto& [ip, cidr] : bans) {
			auto address = boost::asio::ip::address::from_string(ip);

			if(address.is_v6()) {
				throw std::runtime_error("IPv6 bans are not supported but the ban cache encountered one!");
			}

			std::uint32_t mask = (~0U) << (32 - cidr);
			entries_.emplace_back(IPv4Entry{static_cast<std::uint32_t>(address.to_v4().to_ulong()), mask});
		}
	}

public:
	IPBanCache(const std::vector<IPEntry>& bans) {
		load_bans(bans);
	}

	bool is_banned(const std::string& ip) {
		if(entries_.empty()) {
			return false;
		}

		return is_banned(boost::asio::ip::address::from_string(ip));
	}

	bool is_banned(const boost::asio::ip::address& ip) {
		if(entries_.empty()) {
			return false;
		}

		if(ip.is_v6()) {
			return check_ban(ip.to_v6());
		} else {
			return check_ban(ip.to_v4());
		}
	}
};

} // ember