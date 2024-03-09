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

std::string IGDevice::build_http_post_request(std::string&& body,
                                              const std::string& action,
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

	return build_http_request(request);;
}

const std::string& IGDevice::host() const {
	return hostname_;
}

void IGDevice::fetch_device_description(std::shared_ptr<ActionRequest> request) {
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
	request->transport->send(std::move(buffer));
}

void IGDevice::fetch_scpd(std::shared_ptr<ActionRequest> request) {
	auto scpd_uri = get_node_value(service_, "SCPDURL");
	
	if(!scpd_uri) {
		request->callback(false);
		request->transport->close();
		return;
	}

	HTTPRequest http_req {
		.method = "GET",
		.url = *scpd_uri,
		.fields {
			{ "Host",           http_host_ },
			{ "Accept",        R"(text/html,application/xhtml+xml,application/xml;)" },
			{ "Connection",     "keep-alive" },
		}
	};

	auto built = build_http_request(http_req);
	std::vector<std::uint8_t> buffer;
	std::copy(built.begin(), built.end(), std::back_inserter(buffer)); // do better
	request->transport->send(std::move(buffer));
}


void IGDevice::process_request(std::shared_ptr<ActionRequest> request) {
	const auto time = std::chrono::steady_clock::now();
	
	// We're controlling the state here rather than just checking the cache
	// values because we want to avoid a scenario where a device sets a low
	// cache-control value, triggering an infinite GET loop
	if(request->state == ActionState::DEV_DESC_CACHE) {
		if(time > desc_cc_) {
			fetch_device_description(request);
		} else {
			request->state = ActionState::SCPD_CACHE;
		}
	}

	if(request->state == ActionState::SCPD_CACHE) {
		if(time > scpd_cc_) {
			fetch_scpd(request);
		} else {
			request->state = ActionState::ACTION;
		}
	}

	if(request->state == ActionState::ACTION) {
		execute_request(std::move(request));
	}
}

void IGDevice::execute_request(std::shared_ptr<ActionRequest> request) {
	request->handler(*request->transport);
}

void IGDevice::on_connection_error(const boost::system::error_code& ec,
                                   std::shared_ptr<ActionRequest> request) {
	if(ec) {
		request->callback(false);
		request->transport->close();
	}
}

void IGDevice::handle_dev_desc(const HTTPHeader& header, const std::string_view xml) try {
	dev_desc_xml_ = std::move(std::make_unique<XMLParser>(xml));
	desc_cc_ = std::chrono::steady_clock::now();
} catch(std::exception& e) {
	std::cout << e.what(); // todo
}

void IGDevice::handle_scpd(const HTTPHeader& header, std::string_view body) {
	scpd_cc_ = std::chrono::steady_clock::now();
}

void IGDevice::process_action_result(const HTTPHeader& header,
                                     std::shared_ptr<ActionRequest> request) {
	request->callback(header.code == HTTPResponseCode::HTTP_OK);
	request->transport->close();
}

void IGDevice::on_response(const HTTPHeader& header, const std::span<const char> buffer,
                           std::shared_ptr<ActionRequest> request) {
	if(header.code != HTTPResponseCode::HTTP_OK) {
		request->callback(false);
		request->transport->close();
		return;
	}

	const auto length = sv_to_int(header.fields.at("Content-Length")); // todo
	const std::string_view body { buffer.end() - length, buffer.end() };

	switch(request->state) {
		case ActionState::DEV_DESC_CACHE:
			handle_dev_desc(header, body);
			request->state = ActionState::SCPD_CACHE;
			process_request(std::move(request));
			break;
		case ActionState::SCPD_CACHE:
			handle_scpd(header, body);
			request->state = ActionState::ACTION;
			process_request(std::move(request));
			break;
		case ActionState::ACTION:
			process_action_result(header, std::move(request));
			break;
	}
}

void IGDevice::launch_request(std::shared_ptr<ActionRequest> request) {
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

std::optional<std::string> IGDevice::get_node_value(const std::string& service,
                                                    const std::string& node) {
	auto service_node = dev_desc_xml_->locate_service(service);

	if(!service_node) {
		return std::nullopt;
	}

	auto xnode = service_node->first_node(node.c_str(), 0, false);

	if(!xnode  || !xnode ->value()) {
		return std::nullopt;
	}

	return xnode->value();
}

bool IGDevice::delete_port_mapping(Mapping& mapping, HTTPTransport& transport) {
	auto post_uri = get_node_value(service_, "controlURL");

	if(!post_uri) {
		return false;
	}

	auto body = build_upnp_del_mapping(mapping);
	auto request = build_http_post_request(std::move(body), "DeletePortMapping", *post_uri);

	std::vector<std::uint8_t> buffer(request.size());
	std::copy(request.begin(), request.end(), buffer.data());
	transport.send(std::move(buffer));
	return true;
}

bool IGDevice::add_port_mapping(Mapping& mapping, HTTPTransport& transport) {
	auto post_uri = get_node_value(service_, "controlURL");

	if(!post_uri) {
		return false;
	}

	std::string internal_ip = mapping.internal_ip;

	if(mapping.internal_ip.empty()) {
		mapping.internal_ip = transport.local_endpoint().address().to_string();
	}

	auto body = build_upnp_add_mapping(mapping);
	auto request = build_http_post_request(std::move(body), "AddPortMapping", *post_uri);

	std::vector<std::uint8_t> buffer(request.size());
	std::copy(request.begin(), request.end(), buffer.data());
	transport.send(std::move(buffer));
	return true;
}

void IGDevice::map_port(Mapping mapping, Result cb) {
	auto handler = [=, shared_from_this(this)](HTTPTransport& transport) mutable {
		add_port_mapping(mapping, transport);
	};

	auto transport = std::make_unique<HTTPTransport>(ctx_, "0.0.0.0"); // todo

	ActionRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.name = "AddPortMapping",
		.callback = cb
	};

	auto request_ptr = std::make_shared<ActionRequest>(std::move(request));
	launch_request(std::move(request_ptr));
}

void IGDevice::unmap_port(Mapping mapping, Result cb) {
	auto transport = std::make_unique<HTTPTransport>(ctx_, "0.0.0.0"); // todo

	auto handler = [=, shared_from_this(this)](HTTPTransport& transport) mutable {
		delete_port_mapping(mapping, transport);
	};

	ActionRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.name = "DeletePortMapping",
		.callback = cb
	};

	auto request_ptr = std::make_shared<ActionRequest>(std::move(request));
	launch_request(std::move(request_ptr));
}

} // upnp, ports, ember