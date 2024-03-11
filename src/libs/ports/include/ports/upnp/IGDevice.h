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
#include <future>
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

class IGDevice;

struct UPnPRequest {
	using Handler = std::function<ba::awaitable<void>(HTTPTransport&, ErrorCode)>;

	std::unique_ptr<HTTPTransport> transport;
	Handler handler;
	std::shared_ptr<IGDevice> device;
};

constexpr struct use_future_t{} use_future;
constexpr struct use_awaitable_t{} use_awaitable;

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

	void parse_location(const std::string& location);
	ba::awaitable<void> refresh_xml_cache(HTTPTransport& transport);
	ba::awaitable<void> refresh_scpd(HTTPTransport& transport);
	ba::awaitable<void> refresh_igdd(HTTPTransport& transport);
	ba::awaitable<void> request_scpd(HTTPTransport& transport);
	ba::awaitable<void> request_igdd(HTTPTransport& transport);
	ba::awaitable<ErrorCode> do_add_port_mapping(Mapping mapping, HTTPTransport& transport);
	ba::awaitable<ErrorCode> do_delete_port_mapping(Mapping mapping, HTTPTransport& transport);
	ErrorCode validate_soap_arguments(const UPnPActionArgs& args);
	std::string protocol_to_string(const Protocol protocol);

	void launch_request(UPnPRequest::Handler&& handler);
	ba::awaitable<void> process_request(std::shared_ptr<UPnPRequest> request);
	ba::awaitable<ErrorCode> process_request(HTTPTransport& transport, use_awaitable_t);

	template<typename BufType>
	BufType build_http_post_request(std::string&& body, const std::string& action,
	                                const std::string& control_url);

	template<typename BufType>
	BufType build_http_request(const HTTPRequest& request);
	std::string build_soap_request(const UPnPActionArgs&& args);
	std::string_view http_body_view(const HTTPHeader& header, std::span<char> buffer);
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
	std::future<ErrorCode> add_port_mapping(Mapping mapping, use_future_t);
	ba::awaitable<ErrorCode> add_port_mapping(Mapping mapping, use_awaitable_t);

	void delete_port_mapping(Mapping mapping, Result cb);
	std::future<ErrorCode> delete_port_mapping(Mapping mapping, use_future_t);
	ba::awaitable<ErrorCode> delete_port_mapping(Mapping mapping, use_awaitable_t);

	const std::string& host() const;
};

} // upnp, ports, ember