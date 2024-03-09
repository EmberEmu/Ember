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

namespace ember::ports::upnp {

SSDP::SSDP(const std::string& bind, boost::asio::io_context& ctx)
	: ctx_(ctx), strand_(ctx),
	  transport_(ctx, bind, MULTICAST_IPV4_ADDR, DEST_PORT) {
	transport_.set_callbacks(strand_.wrap(
		[&](std::span<const std::uint8_t> datagram,
		    const boost::asio::ip::udp::endpoint&) {
			process_message(datagram);
		})
	);
}

void SSDP::process_message(std::span<const std::uint8_t> datagram) {
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

	auto location = std::string(header.fields["Location"]);
	auto service = std::string(header.fields["ST"]);

	if(location.empty() || service.empty()) {
		return;
	}

	try {
		auto bind = transport_.local_address();
		auto device = std::make_shared<IGDevice>(ctx_, std::move(bind), service, location);

		const bool call_again = handler_(std::move(header), std::move(device));

		if(!call_again) {
			handler_ = {};
		}
	} catch(std::exception&) {}
}

std::vector<std::uint8_t> SSDP::build_ssdp_request(std::string_view type,
                                                   std::string_view subtype,
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

void SSDP::start_ssdp_search(std::string_view type, std::string_view subtype, const int version) {
	auto buffer = build_ssdp_request(type, subtype, version);
	transport_.send(std::move(buffer));
}

void SSDP::locate_gateways(LocateHandler&& handler) {
	handler_ = handler;

	strand_.post([&]() {
		start_ssdp_search("service", "WANIPConnection", 1);
		start_ssdp_search("service", "WANIPConnection", 2);
	});
}

void SSDP::search(std::string_view type, std::string_view subtype,
                  const int version, LocateHandler&& handler) {
	handler_ = handler;

	strand_.post([&]() {
		start_ssdp_search(type, subtype, version);
	});
}

}; // upnp, ports, ember