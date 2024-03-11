/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/Protocol.h>
#include <ports/upnp/HTTPHeaderParser.h>
#include <ports/upnp/HTTPTransport.h>
#include <ports/upnp/XMLParser.h>
#include <ports/upnp/SCPDXMLParser.h>
#include <ports/upnp/ErrorCode.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace ember::ports::upnp {

struct Mapping {
	std::uint16_t external;
	std::uint16_t internal;
	std::uint32_t ttl;
	std::string internal_ip;
	Protocol protocol;
};

struct UPnPActionArgs {
	std::string action;
	std::string service;
	std::vector<std::pair<std::string, std::string>> arguments;
};

using Result = std::function<void(ErrorCode)>;

struct UPnPRequest {
	using Handler = std::function<ba::awaitable<ErrorCode>(HTTPTransport&)>;

	std::unique_ptr<HTTPTransport> transport;
	Handler handler;
	std::string name;
	Result callback;
};

class IGDevice : public std::enable_shared_from_this<IGDevice> {
private:
	boost::asio::io_context& ctx_;
	std::string bind_;

	// device info
	std::string http_host_;
	std::string hostname_;
	std::uint16_t port_;
	std::string dev_desc_uri_;
	std::string service_;

	// document cache control
	std::chrono::steady_clock::time_point igdd_cc_;
	std::chrono::steady_clock::time_point scpd_cc_;
	std::unique_ptr<XMLParser> igdd_xml_;
	std::unique_ptr<SCPDXMLParser> scpd_xml_;

	ba::awaitable<void> refresh_scpd(HTTPTransport& transport);
	ba::awaitable<void> refresh_igdd(HTTPTransport& transport);
	ba::awaitable<ErrorCode> do_add_port_mapping(Mapping& mapping, HTTPTransport& transport);
	ba::awaitable<ErrorCode> do_delete_port_mapping(Mapping& mapping, HTTPTransport& transport);
	void handle_scpd(const HTTPHeader& header, std::string_view body);
	void parse_location(const std::string& location);
	ba::awaitable<void> request_scpd(HTTPTransport& transport);
	ba::awaitable<void> request_device_description(HTTPTransport& transport);
	ErrorCode validate_soap_arguments(const UPnPActionArgs& args);
	std::string protocol_to_string(const Protocol protocol);

	ba::awaitable<void> launch_request(std::shared_ptr<UPnPRequest> request);
	ba::awaitable<void> process_request(std::shared_ptr<UPnPRequest> request);
	std::string_view http_body_from_response(const HTTPTransport::Response& response);

	template<typename BufType>
	BufType build_http_post_request(std::string&& body, const std::string& action,
	                                const std::string& control_url);

	template<typename BufType>
	BufType build_http_request(const HTTPRequest& request);
	std::string build_soap_request(const UPnPActionArgs&& args);
	UPnPActionArgs build_upnp_add_mapping(const Mapping& mapping);
	UPnPActionArgs build_upnp_del_mapping(const Mapping& mapping);

public:
	IGDevice(boost::asio::io_context& ctx_, std::string bind,
	         std::string service, const std::string& location);

	IGDevice(IGDevice&) = default;
	IGDevice(IGDevice&&) = default;
	IGDevice& operator=(IGDevice&) = default;
	IGDevice& operator=(IGDevice&&) = default;

	void add_port_mapping(Mapping mapping, Result cb);
	void delete_port_mapping(Mapping mapping, Result cb);

	const std::string& host() const;
};

} // upnp, ports, ember