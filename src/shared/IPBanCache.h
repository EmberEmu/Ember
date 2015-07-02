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
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <vector>
#include <utility>

namespace ember {

template<typename T>
class IPBanCache {
	T& source_;

	struct IPv4Entry {
		unsigned long range;
		unsigned long mask;
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

	void load_bans() {
		std::vector<IPEntry> results(source_.all_bans());

		for(auto& res : results) {
			auto address = boost::asio::ip::address::from_string(res.first);

			if(address.is_v6()) {
				throw std::runtime_error("IPv6 bans are not supported but the ban cache encountered one!");
			}

			std::uint32_t mask = ~(0xFFFFFFFFu >> res.second);
			entries_.emplace_back(IPv4Entry{address.to_v4().to_ulong(), mask});
		}
	}

public:
	IPBanCache(T& source) : source_(source) {
		load_bans();
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

} //ember