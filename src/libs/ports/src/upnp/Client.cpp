/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/Client.h>
#include <format>
#include <regex>
#include <string>
#include <iostream> // temp

namespace ember::ports::upnp {

Client::Client(const std::string& bind, boost::asio::io_context& ctx)
	: ctx_(ctx), strand_(ctx), transport_(ctx, bind, MULTICAST_IPV4_ADDR, DEST_PORT) {
	transport_.set_callbacks(strand_.wrap(
		[&](std::span<const std::uint8_t> datagram, const boost::asio::ip::udp::endpoint& ep) {
			process_message(datagram, ep);
		})
	);
}

void Client::process_message(std::span<const std::uint8_t> datagram,
                             const boost::asio::ip::udp::endpoint& ep) {
	// we'll need to check the message type later
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

int Client::is_wan_ip_device(const HTTPHeader& header) {
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

void Client::send_map() {
	std::string str = R"(<?xml version="1.0"?>)" "\r\n"
		R"(<s:Envelope)" "\r\n"
		R"(	xmlns:s="http://schemas.xmlsoap.org/soap/envelope/")" "\r\n"
		R"(s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">)" "\r\n"
		R"(	<s:Body>)" "\r\n"
		R"(		<u:actionNameResponse)" "\r\n"
		R"(			xmlns:u="urn:schemas-upnp-org:service:serviceType:v">)" "\r\n"
		R"(			<argumentName>out arg value</argumentName>)" "\r\n"
		R"(			<!-- other out args and their values go here, if any -->)" "\r\n"
		R"(		</u:actionNameResponse>)" "\r\n"
		R"(	</s:Body>)" "\r\n"
		R"(</s:Envelope>)";
}

void Client::send_unmap() {

}

std::vector<std::uint8_t> Client::build_ssdp_request(const std::string& type,
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

void Client::start_ssdp_search() {
	auto buffer = build_ssdp_request("service", "WANIPConnection", 1);
	transport_.send(std::move(buffer));
	buffer = build_ssdp_request("service", "WANIPConnection", 2);
	transport_.send(std::move(buffer));
}

void Client::map_port(const Mapping& mapping, const Gateway& gw) {

}

void Client::unmap_port(const std::uint16_t external_port, const Gateway& gw) {

}

void Client::locate_gateways(LocateHandler&& handler) {
	handler_ = handler;

	strand_.post([&]() {
		start_ssdp_search();
	});
}

}; // upnp, ports, ember