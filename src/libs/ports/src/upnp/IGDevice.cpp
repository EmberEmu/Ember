/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/IGDevice.h>
#include <ports/upnp/Utility.h>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <algorithm>
#include <format>
#include <ranges>
#include <utility>
#include <regex>
#include <cassert>

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
	args.reserve(4096);

	for(auto& [k, v] : action.arguments) {
		args += std::format("<{}>{}</{}>\r\n", k, v, k);
	}

	return std::format(soap, action.action, action.service, args, action.action);
}

std::vector<std::uint8_t> IGDevice::build_http_request(const HTTPRequest& request) {
	std::vector<std::uint8_t> output;
	output.reserve(4096 + request.body.size());

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

const std::string_view IGDevice::protocol_to_string(const Protocol protocol) {
	switch(protocol) {
		case Protocol::tcp:
			return "TCP";
		case Protocol::udp:
			return "UDP";
		default:
			throw std::invalid_argument("Bad protocol specified");
	}
}

UPnPActionArgs IGDevice::build_upnp_add_mapping(const Mapping& mapping) {
	return UPnPActionArgs {
		.action = "AddPortMapping",
		.service = service_,
		.arguments {
			{"NewInternalPort",   std::to_string(mapping.internal)},
			{"NewExternalPort",   std::to_string(mapping.external)},
			{"NewProtocol",       std::string(protocol_to_string(mapping.protocol))},
			{"NewLeaseDuration",  std::to_string(mapping.ttl)},
			{"NewEnabled",        "1"},
			{"NewRemoteHost",     "0"},
			{"NewInternalClient", mapping.internal_ip},
			{"NewPortMappingDescription", "Ember"},
		}
	};
}

UPnPActionArgs IGDevice::build_upnp_del_mapping(const Mapping& mapping) {
	return  UPnPActionArgs {
		.action = "DeletePortMapping",
		.service = service_,
		.arguments {
			{"NewExternalPort",   std::to_string(mapping.external)},
			{"NewProtocol",       std::string(protocol_to_string(mapping.protocol))},
			{"NewRemoteHost",     "0"},
		}
	};
}

std::vector<std::uint8_t> IGDevice::build_http_post_request(const std::string_view body,
                                                            const std::string_view action,
                                                            const std::string_view control_url) {
	const auto soap_action = std::format(R"("{}#{}")", service_, action);
	const auto content_length = std::to_string(body.size());

	const HTTPRequest request {
		.method = "POST",
		.url = control_url,
		.fields {
			{ "Host",           http_host_ },
			{ "Content-Type",   R"(text/xml; charset="utf-8")" },
			{ "Content-Length", content_length },
			{ "Connection",     "keep-alive" },
			{ "SOAPAction",     soap_action },
		},
		.body = body
	};

	return build_http_request(request);
}

asio::awaitable<void> IGDevice::request_igdd(HTTPTransport& transport) {
	HTTPRequest request {
		.method = "GET",
		.url = dev_desc_uri_,
		.fields {
			{ "Host",           http_host_ },
			{ "Accept",         R"(text/html,application/xhtml+xml,application/xml;)" },
			{ "Connection",     "keep-alive" },
		}
	};

	auto buffer = build_http_request(request);
	co_await transport.send(std::move(buffer));
}

asio::awaitable<void> IGDevice::request_scpd(HTTPTransport& transport) {
	assert(igdd_xml_);
	auto scpd_uri = igdd_xml_->get_node_value(service_, "SCPDURL");

	if(!scpd_uri) {
		throw std::invalid_argument("Missing SCPDURL");
	}

	HTTPRequest http_req {
		.method = "GET",
		.url = *scpd_uri,
		.fields {
			{ "Host",           http_host_ },
			{ "Accept",         R"(text/html,application/xhtml+xml,application/xml;)" },
			{ "Connection",     "keep-alive" },
		}
	};

	auto buffer = build_http_request(http_req);
	co_await transport.send(std::move(buffer));
}

asio::awaitable<void> IGDevice::refresh_scpd(HTTPTransport& transport) {
	co_await request_scpd(transport);
	const auto& [header, buffer] = co_await transport.receive_http_response();
	const auto body = http_body_view(header, buffer);
	scpd_cc_ = calculate_cache_expiry(header);
	scpd_xml_ = std::make_unique<SCPDXMLParser>(std::string(body));
}

const std::string_view IGDevice::http_body_view(const HTTPHeader& header, std::span<const char> buffer) {
	if(header.code != HTTPStatus::ok || header.fields.find("Content-Length") == header.fields.end()) {
		throw std::invalid_argument("Bad HTTP response");
	}

	const auto length = sv_to_int(header.fields.at("Content-Length"));

	if(length < 0 || length > buffer.size()) {
		throw std::invalid_argument("Invalid Content-Length");
	}

	return  std::string_view { buffer.end() - length, buffer.end() };
}

std::chrono::steady_clock::time_point IGDevice::calculate_cache_expiry(const HTTPHeader& header) {
	if(auto field = header.fields.find("Cache-Control"); field != header.fields.end()) {
		try {
			const auto value = sv_to_int(extract_field_value(field->second, '='));
			return std::chrono::steady_clock::now() + std::chrono::seconds(value);
		} catch(const std::invalid_argument&) {
			return std::chrono::steady_clock::now();
		}
	}

	return std::chrono::steady_clock::now();
}

asio::awaitable<void> IGDevice::refresh_igdd(HTTPTransport& transport) {
	co_await request_igdd(transport);
	const auto& [header, buffer] = co_await transport.receive_http_response();
	const auto body = http_body_view(header, buffer);
	igdd_cc_ = calculate_cache_expiry(header);
	igdd_xml_ = std::make_unique<XMLParser>(std::string(body));
}

asio::awaitable<void> IGDevice::refresh_xml_cache(HTTPTransport& transport) {
	const auto time = std::chrono::steady_clock::now();

	if(time > igdd_cc_) {
		co_await refresh_igdd(transport);
	}

	if(time > scpd_cc_) {
		co_await refresh_scpd(transport);
	}
}

asio::awaitable<ErrorCode> IGDevice::process_request(HTTPTransport& transport, use_awaitable_t) try {
	co_await transport.connect(hostname_, port_);
	co_await refresh_xml_cache(transport);	
	co_return ErrorCode::success;
} catch(const boost::system::error_code&) {
	co_return ErrorCode::network_failure;
} catch(const std::exception&)  {
	co_return  ErrorCode::http_bad_response;
}

asio::awaitable<void> IGDevice::process_request(std::shared_ptr<UPnPRequest> request) {
	ErrorCode ec { ErrorCode::success };

	try {
		co_await request->transport->connect(hostname_, port_);
		co_await refresh_xml_cache(*request->transport);
	}  catch(const boost::system::error_code&) {
		ec = ErrorCode::network_failure;
	} catch(const std::exception&)  {
		ec = ErrorCode::http_bad_response;
	}

	co_await request->handler(*request->transport, ec);
}


asio::awaitable<ErrorCode> IGDevice::do_delete_port_mapping(const Mapping& mapping,
                                                          HTTPTransport& transport) {
	assert(igdd_xml_);
	const auto post_uri = igdd_xml_->get_node_value(service_, "controlURL");

	if(!post_uri) {
		co_return ErrorCode::soap_missing_uri;
	}

	if(mapping.protocol == Protocol::all) {
		co_return ErrorCode::invalid_mapping_arg;
	}

	const auto args = build_upnp_del_mapping(mapping);

	if(auto ec = validate_soap_arguments(args)) {
		co_return ec;
	}

	const auto body = build_soap_request(args);
	auto request = build_http_post_request(body, "DeletePortMapping", *post_uri);

	co_await transport.send(std::move(request));
	const auto& [header, buffer] = co_await transport.receive_http_response();

	if(header.code != HTTPStatus::ok) {
		co_return ErrorCode::http_not_ok;
	}

	co_return ErrorCode::success;
}

/*
 *  Checks to ensure that all expected action arguments are present in what
 *  we're about to send. However, we do not perform type checking.
 */
ErrorCode IGDevice::validate_soap_arguments(const UPnPActionArgs& args) {
	assert(scpd_xml_);
	const auto& expected_args = scpd_xml_->arguments(args.action, "in");

	if(expected_args.empty()) {
		return ErrorCode::soap_no_arguments;
	}

	for(const auto& arg : expected_args) {
		// didn't originally have a use for a map
		const bool found = std::ranges::any_of(args.arguments | std::views::keys, [&](auto& key) {
			return key == arg;
		});

		if(!found) {
			return ErrorCode::soap_arguments_mismatch;
		}
	}

	return ErrorCode::success;
}

asio::awaitable<ErrorCode> IGDevice::do_add_port_mapping(Mapping mapping, HTTPTransport& transport) {
	assert(igdd_xml_);
	const auto post_uri = igdd_xml_->get_node_value(service_, "controlURL");

	if(!post_uri) {
		co_return ErrorCode::soap_missing_uri;
	}

	if(mapping.internal_ip.empty()) {
		mapping.internal_ip = transport.local_endpoint().address().to_string();
	}

	if(mapping.protocol == Protocol::all) {
		co_return ErrorCode::invalid_mapping_arg;
	}

	const auto args = build_upnp_add_mapping(mapping);

	if(auto ec = validate_soap_arguments(args)) {
		co_return ec;
	}

	const auto body = build_soap_request(args);
	auto request = build_http_post_request(body, "AddPortMapping", *post_uri);

	co_await transport.send(std::move(request));
	const auto& [header, _] = co_await transport.receive_http_response();
	
	if(header.code != HTTPStatus::ok) {
		co_return ErrorCode::http_not_ok;
	}

	co_return ErrorCode::success;
}

void IGDevice::launch_request(UPnPRequest::Handler&& handler) {
	auto transport = std::make_unique<HTTPTransport>(ctx_, bind_);

	UPnPRequest request {
		.transport = std::move(transport),
		.handler = std::move(handler),
		.device = shared_from_this()
	};

	auto request_ptr = std::make_shared<UPnPRequest>(std::move(request));
	asio::co_spawn(ctx_, process_request(std::move(request_ptr)), asio::detached);
}

void IGDevice::add_port_mapping(Mapping mapping, Result cb) {
	auto handler = [&, mapping, cb = std::move(cb)](HTTPTransport& transport,
	                                                ErrorCode ec) -> asio::awaitable<void> {
		if(!ec) {
			auto result = co_await do_add_port_mapping(mapping, transport);
			cb(result);
		} else {
			cb(ec);
		}
	};

	launch_request(std::move(handler));
}

void IGDevice::delete_port_mapping(Mapping mapping, Result cb) {
	auto handler = [&, mapping, cb = std::move(cb)](HTTPTransport& transport,
	                                                ErrorCode ec) -> asio::awaitable<void> {
		if(!ec) {
			auto result = co_await do_delete_port_mapping(mapping, transport);
			cb(result);
		} else {
			cb(ec);
		}
	};

	launch_request(std::move(handler));
}

asio::awaitable<ErrorCode> IGDevice::add_port_mapping(const Mapping& mapping, use_awaitable_t) {
	HTTPTransport transport(ctx_, bind_);

	auto res = co_await process_request(transport, use_awaitable);

	if(!res) {
		co_return res;
	}

	co_return co_await do_add_port_mapping(mapping, transport);
}

std::future<ErrorCode> IGDevice::add_port_mapping(const Mapping& mapping, use_future_t) {
	auto promise = std::make_shared<std::promise<ErrorCode>>();
	auto future = promise->get_future();
	
	add_port_mapping(mapping, [&, promise](ErrorCode ec) {
		promise->set_value(ec);
	});

	return future;
}

asio::awaitable<ErrorCode> IGDevice::delete_port_mapping(Mapping mapping, use_awaitable_t) {
	HTTPTransport transport(ctx_, bind_);
	auto res = co_await process_request(transport, use_awaitable);

	if(!res) {
		co_return res;
	}

	co_return co_await do_delete_port_mapping(mapping, transport);
}

std::future<ErrorCode> IGDevice::delete_port_mapping(Mapping mapping, use_future_t) {
	auto promise = std::make_shared<std::promise<ErrorCode>>();
	auto future = promise->get_future();
	
	delete_port_mapping(mapping, [&, promise](ErrorCode ec) {
		promise->set_value(ec);
	});

	return future;
}

const std::string& IGDevice::host() const {
	return hostname_;
}

} // upnp, ports, ember