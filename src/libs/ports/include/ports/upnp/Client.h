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
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <cstdint>

namespace ember::ports::upnp {

constexpr std::string MULTICAST_IPV4_ADDR { "239.255.255.250" };
constexpr std::string MULTICAST_IPV6_ADDR { "ff05::c" };
constexpr std::uint16_t DEST_PORT { 1900 };

struct Mapping {
	std::uint16_t external;
	std::uint16_t internal;
	std::uint32_t ttl;
};

struct Gateway {
	int version;
	boost::asio::ip::udp::endpoint ep;
protected:
	Gateway(int version, boost::asio::ip::udp::endpoint ep)
		: version(version), ep(ep) {}
	friend class Client;
};

class Client {
public:
	using LocateHandler = std::function<bool(const HTTPHeader&, const Gateway&&)>;

private:
	boost::asio::io_context& ctx_;
	boost::asio::io_context::strand strand_;
	MulticastSocket transport_;
	LocateHandler handler_;

	std::vector<std::uint8_t> build_ssdp_request(const std::string& type,
	                                             const std::string& subtype,
	                                             const int version);

	void process_message(std::span<const std::uint8_t> datagram,
	                     const boost::asio::ip::udp::endpoint& ep);

	void start_ssdp_search();
	int is_wan_ip_device(const HTTPHeader& header);
	void send_map();
	void send_unmap();

public:
	Client(const std::string& bind, boost::asio::io_context& ctx);

	void locate_gateways(LocateHandler&& handler);
	void map_port(const Mapping& mapping, const Gateway& gw);
	void unmap_port(std::uint16_t external_port, const Gateway& gw);
};

}