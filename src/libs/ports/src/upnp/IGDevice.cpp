/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/IGDevice.h>
#include <format>
#include <utility>
#include <regex>

#include <iostream> // todo

namespace ember::ports::upnp {

IGDevice::IGDevice(boost::asio::io_context& ctx, const std::string& location, std::string service)
	: ctx_(ctx), port_(80), service_(std::move(service)) {
	parse_location(location);
}

void IGDevice::parse_location(const std::string& location) {
	constexpr auto pattern = R"((?:http:\/\/)(?:((?:[a-zA-Z0-9.-]*))?(?::((?:[1-9]{1}))"
	                         R"((?:[0-9]{0,4})))?)(\/{1}[a-zA-Z0-9-.]*))";
	std::regex regex(pattern, std::regex::ECMAScript);
	std::smatch matches;

	std::regex_search(location, matches, regex);

	if(matches.size() < 3) {
		throw std::invalid_argument("Invalid location");
	}

	http_host_ = matches[1];
	hostname_ = http_host_;

	if(matches.size() == 3) {
		dev_desc_uri_ = matches[2];
	} else {
		port_ = std::stoi(matches[2]);
		dev_desc_uri_ = matches[3];
	}
}

std::string IGDevice::build_soap_request(const UPnPActionArgs& action) {
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

std::string IGDevice::build_http_request(const HTTPRequest& request) {
	std::string output = std::format("{} {} HTTP/1.1\r\n", request.method, request.url);
	
	for(auto& [k, v]: request.fields) {
		output += std::format("{}: {}\r\n", k, v);
	}

	output += "User-Agent: Ember\r\n";
	output += "\r\n";

	if(!request.body.empty()) {
		output += request.body;
	}
	return output;
}

std::vector<std::uint8_t> IGDevice::build_add_map_action(const Mapping& mapping, const std::string& ip) {
	auto service = dev_desc_xml_->locate_service(service_);

	if(!service) {
		// todo
		return {};
	}

	auto control_url = service->first_node("controlURL", 0, false);

	if(!control_url || !control_url->value()) {
		// todo
		return {};
	}
	
	const UPnPActionArgs args {
		.action = "AddPortMapping",
		.service = service_,
		.arguments {
			{"NewInternalPort",   std::to_string(mapping.internal)},
			{"NewExternalPort",   std::to_string(mapping.external)},
			{"NewProtocol",       "TCP"},
			{"NewLeaseDuration",  std::to_string(mapping.ttl)},
			{"NewEnabled",        "1"},
			{"NewRemoteHost",     "0"},
			{"NewInternalClient", ip},
			{"NewPortMappingDescription", "Ember"},
		}
	};

	auto body = build_soap_request(args);
	auto soap_action = std::format("{}#{}", service_, args.action);

	HTTPRequest request {
		.method = "POST",
		.url = control_url->value(),
		.fields {
			{ "Host",           http_host_ },
			{ "Content-Type",   R"(text/xml; charset="utf-8")" },
			{ "Content-Length", std::to_string(body.size()) },
			{ "Connection",     "keep-alive" },
			{ "SOAPAction",     std::move(soap_action) },
		},
		.body = std::move(body)
	};
	
	auto built = build_http_request(request);
	std::vector<std::uint8_t> buffer;
	std::copy(built.begin(), built.end(), std::back_inserter(buffer));
	return buffer;
}

const std::string& IGDevice::host() const {
	return hostname_;
}

void IGDevice::fetch_device_description(ActionRequest&& request) {
	HTTPRequest http_req {
		.method = "GET",
		.url = dev_desc_uri_,
		.fields {
			{ "Host",           http_host_ },
			{ "Accept",        R"(text/html,application/xhtml+xml,application/xml;)" },
			{ "Connection",     "keep-alive" },
		}
	};

	auto built = build_http_request(http_req);
	std::vector<std::uint8_t> buffer;
	std::copy(built.begin(), built.end(), std::back_inserter(buffer)); // do better
	request.transport->send(std::move(buffer));
}

void IGDevice::process_request(ActionRequest&& request) {
	const auto time = std::chrono::steady_clock::now();
	
	// We're controlling the state here rather than just checking the cache
	// values because we want to avoid a scenario where a device sets a low
	// cache-control value, triggering an infinite GET loop
	if(request.state == ActionState::DEV_DESC_CACHE) {
		if(time > desc_cc_) {
			fetch_device_description(std::move(request));
		} else {
			request.state = ActionState::SCPD_CACHE;
		}
	}

	if(request.state == ActionState::SCPD_CACHE) {
		if(false) {
			//fetch_scpd(std::move(request));
		} else {
			request.state = ActionState::ACTION;
		}
	}

	if(request.state == ActionState::ACTION) {
		execute_request(std::move(request));
	}
}

void IGDevice::execute_request(ActionRequest&& request) {
	request.handler();
}

void IGDevice::on_connection_error(const boost::system::error_code& ec,
                                 ActionRequest request) {
	if(ec) {
		// todo
	}
}

void IGDevice::handle_dev_desc(const HTTPHeader& header, const std::string_view xml) try {
	dev_desc_xml_ = std::move(std::make_unique<XMLParser>(xml));
	desc_cc_ = std::chrono::steady_clock::now();
} catch(std::exception& e) {
	std::cout << e.what(); // todo
}

void IGDevice::on_response(const HTTPHeader& header, const std::span<const char> buffer,
                         ActionRequest request) {
	const auto length = sv_to_int(header.fields.at("Content-Length")); // todo
	const std::string_view body { buffer.end() - length, buffer.end() };

	switch(request.state) {
		case ActionState::DEV_DESC_CACHE:
			handle_dev_desc(header, body);
			request.state = ActionState::SCPD_CACHE;
			process_request(std::move(request));
			break;
		case ActionState::SCPD_CACHE:
			request.state = ActionState::ACTION;
			process_request(std::move(request));
			break;
		case ActionState::ACTION:
			break;
	}
}

void IGDevice::launch_request(ActionRequest&& request) {
	auto shared = shared_from_this();

	request.transport->set_callbacks(
		[&, shared, request](const HTTPHeader& header, std::span<const char> buffer) {
			on_response(header, buffer, request);
		},
		[&, shared, request](const boost::system::error_code& ec) {
			on_connection_error(ec, request);
		}
	);

	request.transport->connect(hostname_, port_,
		[&, request, shared](const boost::system::error_code& ec) mutable {
			if(!ec) {
				process_request(std::move(request));
			}
		}
	);
}

void IGDevice::do_port_mapping(const Mapping& mapping, HTTPTransport& transport) {
	std::string internal_ip = mapping.internal_ip;

	if(internal_ip.empty()) {
		internal_ip = transport.local_endpoint().address().to_string();
	}

	auto buffer = build_add_map_action(mapping, internal_ip);
	transport.send(std::move(buffer));
}

void IGDevice::map_port(const Mapping& mapping) {
	auto transport = std::make_shared<HTTPTransport>(ctx_, "0.0.0.0"); // todo

	auto handler = [=, shared_from_this(this)] {
		do_port_mapping(mapping, *transport);
	};

	ActionRequest request{
		.transport = transport,
		.handler = std::move(handler),
		.name = "AddPortMapping"
	};
	
	launch_request(std::move(request));
}

void IGDevice::unmap_port(const std::uint16_t external_port) {

}


} // upnp, ports, ember