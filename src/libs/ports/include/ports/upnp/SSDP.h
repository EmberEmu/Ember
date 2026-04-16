/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/Protocol.h>
#include <ports/upnp/MulticastSocket.h>
#include <ports/upnp/HTTPTypes.h>
#include <ports/upnp/IGDevice.h>
#include <ports/upnp/ErrorCode.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace ember::ports::upnp {

constexpr std::string_view mcast_ipv4_addr { "239.255.255.250" };
constexpr std::string_view mcast_ipv6_addr { "ff05::c" };
constexpr std::uint16_t mcast_dest_port { 1900 };

struct DeviceResult {
	HTTPHeader header;
	std::shared_ptr<IGDevice> device;
};

using LocateResult = std::expected<DeviceResult, ErrorCode>;
using LocateHandler = std::function<bool(LocateResult)>;

class SSDP final {
	boost::asio::io_context& ctx_;
	boost::asio::io_context::strand strand_;
	MulticastSocket transport_;
	LocateHandler handler_;

	std::vector<std::uint8_t> build_ssdp_request(const std::string_view type,
	                                             std::string_view subtype,
	                                             const int version);

	ErrorCode validate_message(std::span<const std::uint8_t> datagram);
	boost::asio::awaitable<void> start_ssdp_search(const std::string_view type, const std::string_view subtype, int version);
	boost::asio::awaitable<void> read_broadcasts();
	LocateResult build_locate_result(std::span<const std::uint8_t> datagram);

public:
	SSDP(const std::string& bind, boost::asio::io_context& ctx);

	void locate_gateways(LocateHandler&& handler);
	boost::asio::awaitable<LocateResult> locate_gateways(use_awaitable_t);
	std::future<LocateResult> locate_gateways(use_future_t);

	void search(const std::string& type, const std::string& subtype, int version, LocateHandler&& handler);
};

} // upnp, ports, ember