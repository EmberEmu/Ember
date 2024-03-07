/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/Device.h>


namespace ember::ports::upnp {

std::string Device::build_soap_request(const UPnPActionArgs& action) {
	constexpr std::string_view soap =
		R"(<?xml version="1.0"?>)" "\r\n"
		R"(<s:Envelope)" "\r\n"
		R"(	xmlns:s="http://schemas.xmlsoap.org/soap/envelope/")" "\r\n"
		R"(s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">)" "\r\n"
		R"(	<s:Body>)" "\r\n"
		R"(		<u:{})" "\r\n"
		R"(			xmlns:u="urn:schemas-upnp-org:service:{}">)" "\r\n"
		R"(			{})" "\r\n"
		R"(		</u:{}>)" "\r\n"
		R"(	</s:Body>)" "\r\n"
		R"(</s:Envelope>)";

	std::string args;

	for(auto& [k, v] : action.arguments) {
		args += std::format("<{}>{}</{}>\r\n", k, v, k);
	}

	return std::format(soap, action.action, action.service, args, action.action);
}

std::string Device::build_http_request(const HTTPRequest& request) {
	std::string output = std::format("{} {} HTTP/1.1\r\n", request.method, request.url);
	
	for(auto& [k, v]: request.fields) {
		output += std::format("{}: {}\r\n", k, v);
	}

	output += request.body;
	return output;
}

void Device::send_map(const Mapping& mapping, const Gateway& gw) {
	const UPnPActionArgs args {
		.action = "AddPortMapping",
		.service = "urn:schemas-upnp-org:service:WANIPConnection:2",
		.arguments {
			{"InternalPort",     "3724"},
			{"ExternalPort",     "3724"},
			{"NewProtocol",      "TCP"},
			{"NewLeaseDuration", "0"},
			{"NewEnabled",       "1"}
		}
	};

	auto body = build_soap_request(args);

	HTTPRequest request {
		.method = "POST",
		.url = "/igdupnp/control/WANIPConn1",
		.fields {
			{ "Host",           gw.ep.address().to_string() },
			{ "Content-Type",   R"(text/xml; charset="utf-8")" },
			{ "Content-Length", std::to_string(body.size()) },
			{ "Connection",     "close" },
			{ "SOAPAction",     "urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping" },
		},
		.body = std::move(body)
	};
	
	auto built = build_http_request(request);
	std::vector<std::uint8_t> buffer;
	std::copy(built.begin(), built.end(), std::back_inserter(buffer));
	//transport_.send(std::move(buffer), gw.ep);
}

void Device::send_unmap() {

}


void Device::map_port(const Mapping& mapping, const Gateway& gw) {
	send_map(mapping, gw);
}

void Device::unmap_port(const std::uint16_t external_port, const Gateway& gw) {

}


} // upnp, ports, ember