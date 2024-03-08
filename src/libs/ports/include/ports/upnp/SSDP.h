/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/MulticastSocket.h>
#include <ports/upnp/HTTPHeaderParser.h>
#include <ports/upnp/IGDevice.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <cstdint>

namespace ember::ports::upnp {

constexpr std::string MULTICAST_IPV4_ADDR { "239.255.255.250" };
constexpr std::string MULTICAST_IPV6_ADDR { "ff05::c" };
constexpr std::uint16_t DEST_PORT { 1900 };

class SSDP {
public:
	using LocateHandler = std::function<bool(const HTTPHeader&&, std::shared_ptr<IGDevice>)>;

private:
	boost::asio::io_context& ctx_;
	boost::asio::io_context::strand strand_;
	MulticastSocket transport_;
	LocateHandler handler_;

	std::vector<std::uint8_t> build_ssdp_request(std::string_view type,
												 std::string_view subtype,
												 const int version);

	void process_message(std::span<const std::uint8_t> datagram,
	                     const boost::asio::ip::udp::endpoint& ep);

	void start_ssdp_search(std::string_view type, std::string_view subtype, int version);
	int is_wan_ip_device(const HTTPHeader& header);

public:
	SSDP(const std::string& bind, boost::asio::io_context& ctx);

	void locate_gateways(LocateHandler&& handler);

	void search(std::string_view type, std::string_view subtype,
				int version, LocateHandler&& handler);
};

} // upnp, ports, ember