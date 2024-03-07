/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/SSDP.h>
#include <format>
#include <regex>
#include <string>
#include <iostream> // temp

namespace ember::ports::upnp {

SSDP::SSDP(const std::string& bind, boost::asio::io_context& ctx)
	: ctx_(ctx), strand_(ctx),
	  transport_(ctx, bind, MULTICAST_IPV4_ADDR, DEST_PORT) {
	transport_.set_callbacks(strand_.wrap(
		[&](std::span<const std::uint8_t> datagram, const boost::asio::ip::udp::endpoint& ep) {
			process_message(datagram, ep);
		})
	);
}

void SSDP::process_message(std::span<const std::uint8_t> datagram,
                           const boost::asio::ip::udp::endpoint& ep) {
	if(!handler_) {
		return;
	}

	std::string_view txt(
		reinterpret_cast<const char*>(datagram.data()), datagram.size()
	);

	HTTPHeader header;
	
	if(!parse_http_header(txt, header)) {
		return;
	}

	if(header.code != HTTPResponseCode::HTTP_OK) {
		return;
	}

	const auto version = is_wan_ip_device(header);

	if(!version) {
		return;
	}

	const bool call_again = handler_(header, { version, ep });

	if(!call_again) {
		handler_ = {};
	}
}

int SSDP::is_wan_ip_device(const HTTPHeader& header) {
	for(auto& [k, v] : header.fields) {
		if(k != "ST") { // pretty lazy but it'll do until it won't
			continue;
		}

		if(v == "urn:schemas-upnp-org:service:WANIPConnection:1") {
			return 1;
		} else if(v == "urn:schemas-upnp-org:service:WANIPConnection:2") {
			return 2;
		}
	}

	return 0;
}

std::vector<std::uint8_t> SSDP::build_ssdp_request(const std::string& type,
                                                   const std::string& subtype,
                                                   const int version) {
	constexpr std::string_view request {
		R"(M-SEARCH * HTTP/1.1")" "\r\n"
		R"(MX: 2)" "\r\n"
		R"(HOST: {}:{})" "\r\n"
		R"(MAN: "ssdp:discover")" "\r\n"
		R"(ST: urn:schemas-upnp-org:{}:{}:{})" "\r\n"
	};

	std::vector<std::uint8_t> buffer;
	std::format_to(std::back_inserter(buffer), request, MULTICAST_IPV4_ADDR,
	               DEST_PORT, type, subtype, version);
	return buffer;
}

void SSDP::start_ssdp_search() {
	auto buffer = build_ssdp_request("service", "WANIPConnection", 1);
	transport_.send(std::move(buffer));
	buffer = build_ssdp_request("service", "WANIPConnection", 2);
	transport_.send(std::move(buffer));
}

void SSDP::locate_gateways(LocateHandler&& handler) {
	handler_ = handler;

	strand_.post([&]() {
		start_ssdp_search();
	});
}

}; // upnp, ports,