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

std::string IGDevice::build_soap_request(const UPnPActionArgs&& action) {
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

UPnPActionArgs IGDevice::build_upnp_add_mapping(const Mapping& mapping) {
	std::string protocol = mapping.protocol == Protocol::PROTO_TCP? "TCP" : "UDP";

	UPnPActionArgs args {
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

	return args;
}

UPnPActionArgs IGDevice::build_upnp_del_mapping(const Mapping& mapping) {
	std::string protocol = mapping.protocol == Protocol::PROTO_TCP? "TCP" : "UDP";

	UPnPActionArgs args {
		.action = "DeletePortMapping",
		.service = service_,
		.arguments {
			{"NewExternalPort",   std::to_string(mapping.external)},
			{"NewProtocol",       protocol},
			{"NewRemoteHost",     "0"},	
		}
	};

	return args;
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

ba::awaitable<void> IGDevice::request_device_description(HTTPTransport& transport) {
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
	co_await transport.send(std::move(buffer));
}

ba::awaitable<void> IGDevice::request_scpd(HTTPTransport& transport) {
	auto scpd_uri = igdd_xml_->get_node_value(service_, "SCPDURL");

	if(!scpd_uri) {
		throw std::invalid_argument("Missing SCPDURL");
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
	co_await transport.send(std::move(buffer));
}

ba::awaitable<void> IGDevice::refresh_scpd(HTTPTransport& transport) {
	co_await request_scpd(transport);
	const auto response = co_await transport.receive_http_response();
	const auto body = http_body_from_response(response);
	const auto& [header, buffer] = response;

	if(auto field = header.fields.find("Cache-Control"); field != header.fields.end()) {
		try {
			const auto value = sv_to_int(split_argument(field->second, '='));
			scpd_cc_ = std::chrono::steady_clock::now() + std::chrono::seconds(value);
		} catch(std::invalid_argument&) {
			scpd_cc_ = std::chrono::steady_clock::now();
		}
	} else {
		scpd_cc_ = std::chrono::steady_clock::now();
	}

	// we don't actually care about this XML but we might in the future (x to doubt)
	scpd_xml_ = std::move(std::make_unique<SCPDXMLParser>(body));
}

std::string_view IGDevice::http_body_from_response(const HTTPTransport::Response& response) {
	const auto& [header, buffer] = response;

	if(header.code != HTTPResponseCode::HTTP_OK 
	   || header.fields.find("Content-Length") == header.fields.end()) {
		throw std::invalid_argument("Bad HTTP response");
	}

	const auto length = sv_to_int(header.fields.at("Content-Length"));
	const std::string_view body { buffer.end() - length, buffer.end() };
	return body;
}

ba::awaitable<void> IGDevice::refresh_igdd(HTTPTransport& transport) {
	co_await request_device_description(transport);
	const auto response = co_await transport.receive_http_response();
	const auto& [header, buffer] = response;
	const auto body = http_body_from_response(response);

	if(auto field = header.fields.find("Cache-Control"); field != header.fields.end()) {
		try {
			const auto value = sv_to_int(split_argument(field->second, '='));
			igdd_cc_ = std::chrono::steady_clock::now() + std::chrono::seconds(value);
		} catch(std::invalid_argument&) {
			igdd_cc_ = std::chrono::steady_clock::now();
		}
	} else {
		igdd_cc_ = std::chrono::steady_clock::now();
	}
	
	igdd_xml_ = std::move(std::make_unique<XMLParser>(body));
}

ba::awaitable<void> IGDevice::process_request(std::shared_ptr<UPnPRequest> request) try {
	const auto time = std::chrono::steady_clock::now();

	if(time > igdd_cc_) {
		co_await refresh_igdd(*request->transport);
	}

	if(time > scpd_cc_) {
		co_await refresh_scpd(*request->transport);
	}

	auto result = co_await request->handler(*request->transport);
	request->callback(result);
} catch(boost::system::error_code&) {
	request->callback(ErrorCode::NETWORK_FAILURE);
} catch(std::exception&)  {
	request->callback(ErrorCode::HTTP_BAD_RESPONSE);
}

ba::awaitable<void> IGDevice::launch_request(std::shared_ptr<UPnPRequest> request) {
	auto shared = shared_from_this();
	co_await request->transport->connect(hostname_, port_);
	co_await process_request(request);
}

ba::awaitable<ErrorCode> IGDevice::delete_port_mapping(Mapping& mapping, HTTPTransport& transport) {
	auto post_uri = igdd_xml_->get_node_value(service_, "controlURL");

	if(!post_uri) {
		co_return ErrorCode::SOAP_MISSING_URI;
	}

	auto args = build_upnp_del_mapping(mapping);

	if(auto result = validate_soap_arguments(args); result != ErrorCode::SUCCESS) {
		co_return result;
	}

	auto body = build_soap_request(std::move(args));

	auto request = build_http_post_request<std::vector<std::uint8_t>>(
		std::move(body), "DeletePortMapping", *post_uri
	);

	co_await transport.send(std::move(request));
	const auto& [header, buffer] = co_await transport.receive_http_response();

	if(header.code != HTTPResponseCode::HTTP_OK) {
		co_return ErrorCode::HTTP_NOT_OK;
	}

	co_return ErrorCode::SUCCESS;
}

/*
   Checks to ensure that all expected action arguments are present in what
   we're about to send. However, we do not perform type checking.
 */
ErrorCode IGDevice::validate_soap_arguments(const UPnPActionArgs& args) {
	const auto& expected_args = scpd_xml_->arguments(args.action, "in");

	if(expected_args.empty()) {
		return ErrorCode::SOAP_NO_ARGUMENTS;
	}

	for(const auto& arg : expected_args) {
		bool found = false;

		// didn't originally have a use for a map
		for(const auto& [k, v]: args.arguments) {
			if(k == arg) {
				found = true;
				break;
			}
		}

		if(!found) {
			return ErrorCode::SOAP_ARGUMENTS_MISMATCH;
		}
	}

	return ErrorCode::SUCCESS;
}

ba::awaitable<ErrorCode> IGDevice::add_port_mapping(Mapping& mapping, HTTPTransport& transport) {
	auto post_uri = igdd_xml_->get_node_value(service_, "controlURL");

	if(!post_uri) {
		co_return ErrorCode::SOAP_MISSING_URI;
	}

	if(mapping.internal_ip.empty()) {
		mapping.internal_ip = transport.local_endpoint().address().to_string();
	}

	auto args = build_upnp_add_mapping(mapping);

	if(auto result = validate_soap_arguments(args); result != ErrorCode::SUCCESS) {
		co_return result;
	}

	auto body = build_soap_request(std::move(args));

	auto request = build_http_post_request<std::vector<std::uint8_t>>(
		std::move(body), "AddPortMapping", *post_uri
	);

	co_await transport.send(std::move(request));
	const auto& [header, buffer] = co_await transport.receive_http_response();
	
	if(header.code != HTTPResponseCode::HTTP_OK) {
		co_return ErrorCode::HTTP_NOT_OK;
	}

	co_return ErrorCode::SUCCESS;
}

void IGDevice::map_port(Mapping mapping, Result cb) {
	auto shared = shared_from_this();

	auto handler = [=, this](HTTPTransport& transport) mutable -> ba::awaitable<ErrorCode> {
		co_return co_await add_port_mapping(mapping, transport);
	};

	auto transport = std::make_unique<HTTPTransport>(ctx_, bind_);

	UPnPRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.name = "AddPortMapping",
		.callback = cb
	};

	auto request_ptr = std::make_shared<UPnPRequest>(std::move(request));
	auto executor = ctx_.get_executor();

	boost::asio::dispatch(executor, [=, this]() mutable {
		ba::co_spawn(ctx_, launch_request(request_ptr), ba::detached);
	});
}

void IGDevice::unmap_port(Mapping mapping, Result cb) {
	auto shared = shared_from_this();

	auto handler = [=, this](HTTPTransport& transport) mutable -> ba::awaitable<ErrorCode> {
		co_return co_await delete_port_mapping(mapping, transport);
	};

	auto transport = std::make_unique<HTTPTransport>(ctx_, bind_);

	UPnPRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.name = "DeletePortMapping",
		.callback = cb
	};

	auto request_ptr = std::make_shared<UPnPRequest>(std::move(request));
	auto executor = ctx_.get_executor();

	boost::asio::dispatch(executor, [=, this]() mutable {
		ba::co_spawn(ctx_, launch_request(request_ptr), ba::detached);
	});
}

const std::string& IGDevice::host() const {
	return hostname_;
}

} // upnp, ports, ember