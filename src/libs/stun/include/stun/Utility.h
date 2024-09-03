/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Protocol.h>
#include <boost/asio/ip/address.hpp>
#include <algorithm>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

namespace ember::stun {

std::string extract_ip_to_string(const auto& address) {
	std::string addr_str;

	if(address.family == AddressFamily::IPV4) {
		addr_str = boost::asio::ip::address_v4(address.ipv4).to_string();
	} else if(address.family == AddressFamily::IPV6) {
		addr_str = boost::asio::ip::address_v6(address.ipv6).to_string();
	} else {
		throw std::invalid_argument("Unable to extract address");
	}

	return addr_str;
}

template<typename T>
std::optional<T> retrieve_attribute(std::span<const attributes::Attribute> attrs) {
	for(const auto& attr : attrs) {
		if(const T* val = std::get_if<T>(&attr)) {
			return *val;
		}
	}

	return std::nullopt;
}

} // stun, ember