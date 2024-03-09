/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/IGDevice.h>
#include <ports/upnp/Utility.h>
#include <format>
#include <utility>
#include <regex>

namespace ember::ports::upnp {

IGDevice::IGDevice(boost::asio::io_context& ctx, std::string bind,
                   std::string service, const std::string& location)
	: ctx_(ctx), port_(80), service_(std::move(service)), bind_(std::move(bind)) {
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

template<typename BufType>
BufType IGDevice::build_http_request(const HTTPRequest& request) {
	BufType output;
	std::format_to(std::back_inserter(output), "{} {} HTTP/1.1\r\n", request.method, request.url);
	
	for(auto& [k, v]: request.fields) {
		std::format_to(std::back_inserter(output), "{}: {}\r\n", k, v);
	}

	std::format_to(std::back_inserter(output), "User-Agent: Ember\r\n\r\n");

	if(!request.body.empty()) {
		std::format_to(std::back_inserter(output), "{}", request.body);
	}

	return output;
}

std::string IGDevice::build_upnp_add_mapping(const Mapping& mapping) {
	std::string protocol = mapping.protocol == Protocol::PROTO_TCP? "TCP" : "UDP";

	const UPnPActionArgs args {
		.action = "AddPortMapping",
		.service = service_,
		.arguments {
			{"NewInternalPort",   std::to_string(mapping.internal)},
			{"NewExternalPort",   std::to_string(mapping.external)},
			{"NewProtocol",       protocol},
			{"NewLeaseDuration",  std::to_string(mapping.ttl)},
			{"NewEnabled",        "1"},
			{"NewRemoteHost",     "0"},
			{"NewInternalClient", mapping.internal_ip},
			{"NewPortMappingDescription", "Ember"},
		}
	};

	return build_soap_request(args);
}

std::string IGDevice::build_upnp_del_mapping(const Mapping& mapping) {
	std::string protocol = mapping.protocol == Protocol::PROTO_TCP? "TCP" : "UDP";

	const UPnPActionArgs args {
		.action = "DeletePortMapping",
		.service = service_,
		.arguments {
			{"NewExternalPort",   std::to_string(mapping.external)},
			{"NewProtocol",       protocol},
			{"NewRemoteHost",     "0"},	
		}
	};

	return build_soap_request(args);
}

template<typename BufType>
BufType IGDevice::build_http_post_request(std::string&& body, const std::string& action,
                                          const std::string& control_url) {
	auto soap_action = std::format("{}#{}", service_, action);

	HTTPRequest request {
		.method = "POST",
		.url = control_url,
		.fields {
			{ "Host",           http_host_ },
			{ "Content-Type",   R"(text/xml; charset="utf-8")" },
			{ "Content-Length", std::to_string(body.size()) },
			{ "Connection",     "keep-alive" },
			{ "SOAPAction",     std::move(soap_action) },
		},
		.body = std::move(body)
	};

	return build_http_request<BufType>(request);
}

void IGDevice::fetch_device_description(std::shared_ptr<UPnPRequest> request) {
	HTTPRequest http_req{
		.method = "GET",
		.url = dev_desc_uri_,
		.fields {
			{ "Host",           http_host_ },
			{ "Accept",        R"(text/html,application/xhtml+xml,application/xml;)" },
			{ "Connection",     "keep-alive" },
		}
	};

	auto buffer = build_http_request<std::vector<std::uint8_t>>(http_req);
	request->transport->send(std::move(buffer));
}

void IGDevice::fetch_scpd(std::shared_ptr<UPnPRequest> request) {
	auto scpd_uri = igdd_xml_->get_node_value(service_, "SCPDURL");

	if(!scpd_uri) {
		request->callback(false);
		request->transport.reset();
		return;
	}

	HTTPRequest http_req{
		.method = "GET",
		.url = *scpd_uri,
		.fields {
			{ "Host",           http_host_ },
			{ "Accept",        R"(text/html,application/xhtml+xml,application/xml;)" },
			{ "Connection",     "keep-alive" },
		}
	};

	auto buffer = build_http_request<std::vector<std::uint8_t>>(http_req);
	request->transport->send(std::move(buffer));
}

void IGDevice::process_request(std::shared_ptr<UPnPRequest> request) {
	const auto time = std::chrono::steady_clock::now();

	// We're controlling the state here rather than just checking the cache
	// values because we want to avoid a scenario where a device sets a low
	// cache-control value, triggering an infinite GET loop
	if(request->state == UPnPRequest::State::IGDD_CACHE) {
		if(time > igdd_cc_) {
			fetch_device_description(request);
		} else {
			request->state = UPnPRequest::State::SCPD_CACHE;
		}
	}

	if(request->state == UPnPRequest::State::SCPD_CACHE) {
		if(time > scpd_cc_) {
			fetch_scpd(request);
		} else {
			request->state = UPnPRequest::State::ACTION;
		}
	}

	if(request->state == UPnPRequest::State::ACTION) {
		execute_request(std::move(request));
	}
}

void IGDevice::execute_request(std::shared_ptr<UPnPRequest> request) {
	request->handler(*request->transport);
}

void IGDevice::on_connection_error(const boost::system::error_code& ec,
								   std::shared_ptr<UPnPRequest> request) {
	if(ec) {
		request->callback(false);
		request->transport.reset();
	}
}

void IGDevice::handle_igdd(const HTTPHeader& header, const std::string_view xml) {
	igdd_xml_ = std::move(std::make_unique<XMLParser>(xml));
	igdd_cc_ = std::chrono::steady_clock::now();
}

void IGDevice::handle_scpd(const HTTPHeader& header, std::string_view body) {
	scpd_cc_ = std::chrono::steady_clock::now();
}

void IGDevice::process_action_result(const HTTPHeader& header,
									 std::shared_ptr<UPnPRequest> request) {
	request->callback(header.code == HTTPResponseCode::HTTP_OK);
	request->transport.reset();
}

void IGDevice::on_response(const HTTPHeader& header, const std::span<const char> buffer,
						   std::shared_ptr<UPnPRequest> request) try {
	if(header.code != HTTPResponseCode::HTTP_OK 
	   || header.fields.find("Content-Length") == header.fields.end()) {
		request->callback(false);
		request->transport.reset();
		return;
	}

	const auto length = sv_to_int(header.fields.at("Content-Length"));
	const std::string_view body { buffer.end() - length, buffer.end() };

	switch(request->state) {
		case UPnPRequest::State::IGDD_CACHE:
			handle_igdd(header, body);
			request->state = UPnPRequest::State::SCPD_CACHE;
			process_request(std::move(request));
			break;
		case UPnPRequest::State::SCPD_CACHE:
			handle_scpd(header, body);
			request->state = UPnPRequest::State::ACTION;
			process_request(std::move(request));
			break;
		case UPnPRequest::State::ACTION:
			process_action_result(header, std::move(request));
			break;
	}
} catch(std::exception&) {
	request->callback(false);
	request->transport.reset();
}

void IGDevice::launch_request(std::shared_ptr<UPnPRequest> request) {
	auto shared = shared_from_this();

	request->transport->set_callbacks(
		[&, shared, request](const HTTPHeader& header, std::span<const char> buffer) {
			on_response(header, buffer, request);
		},
		[&, shared, request](const boost::system::error_code& ec) {
			on_connection_error(ec, request);
		}
	);

	request->transport->connect(hostname_, port_,
		[&, request, shared](const boost::system::error_code& ec) mutable {
			if(!ec) {
				process_request(request);
			}
		}
	);
}

bool IGDevice::delete_port_mapping(Mapping& mapping, HTTPTransport& transport) {
	auto post_uri = igdd_xml_->get_node_value(service_, "controlURL");

	if(!post_uri) {
		return false;
	}

	auto body = build_upnp_del_mapping(mapping);
	auto request = build_http_post_request<std::vector<std::uint8_t>>(
		std::move(body), "DeletePortMapping", *post_uri
	);

	transport.send(std::move(request));
	return true;
}

bool IGDevice::add_port_mapping(Mapping& mapping, HTTPTransport& transport) {
	auto post_uri = igdd_xml_->get_node_value(service_, "controlURL");

	if(!post_uri) {
		return false;
	}

	std::string internal_ip = mapping.internal_ip;

	if(mapping.internal_ip.empty()) {
		mapping.internal_ip = transport.local_endpoint().address().to_string();
	}

	auto body = build_upnp_add_mapping(mapping);
	auto request = build_http_post_request<std::vector<std::uint8_t>>(
		std::move(body), "AddPortMapping", *post_uri
	);

	transport.send(std::move(request));
	return true;
}

void IGDevice::map_port(Mapping mapping, Result cb) {
	auto handler = [=, shared_from_this(this)](HTTPTransport& transport) mutable {
		add_port_mapping(mapping, transport);
	};

	auto transport = std::make_unique<HTTPTransport>(ctx_, bind_);

	UPnPRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.name = "AddPortMapping",
		.callback = cb
	};

	auto request_ptr = std::make_shared<UPnPRequest>(std::move(request));
	launch_request(std::move(request_ptr));
}

void IGDevice::unmap_port(Mapping mapping, Result cb) {
	auto transport = std::make_unique<HTTPTransport>(ctx_, bind_);

	auto handler = [=, shared_from_this(this)](HTTPTransport& transport) mutable {
		delete_port_mapping(mapping, transport);
	};

	UPnPRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.name = "DeletePortMapping",
		.callback = cb
	};

	auto request_ptr = std::make_shared<UPnPRequest>(std::move(request));
	launch_request(std::move(request_ptr));
}

const std::string& IGDevice::host() const {
	return hostname_;
}

} // upnp, ports, ember