/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/HTTPHeaderParser.h>
#include <ports/upnp/HTTPTransport.h>
#include <ports/upnp/XMLParser.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace ember::ports::upnp {

struct Mapping {
	std::uint16_t external;
	std::uint16_t internal;
	std::uint32_t ttl;
	std::string internal_ip;
};

struct UPnPActionArgs {
	std::string action;
	std::string service;
	std::vector<std::pair<std::string, std::string>> arguments;
};

enum class ActionState {
	DEV_DESC_CACHE, SCPD_CACHE, ACTION, DONE
};

struct ActionRequest {
	using Handler = std::function<void()>;
	std::shared_ptr<HTTPTransport> transport;
	ActionState state = ActionState::DEV_DESC_CACHE;
	Handler handler;
	std::string name;
};

class IGDevice : public std::enable_shared_from_this<IGDevice> {
	boost::asio::io_context& ctx_;

	// device info
	std::string http_host_;
	std::string hostname_;
	std::uint16_t port_;
	std::string dev_desc_uri_;
	std::string service_;

	// document cache control
	std::chrono::steady_clock::time_point desc_cc_;
	std::chrono::steady_clock::time_point scpd_cc_;
	std::unique_ptr<XMLParser> dev_desc_xml_;

	void parse_location(const std::string& location);
	void launch_request(ActionRequest&& request);
	void process_request(ActionRequest&& request);
	void fetch_device_description(ActionRequest&& request);
	void handle_dev_desc(const HTTPHeader& header, const std::string_view xml);
	void execute_request(ActionRequest&& request);
	std::vector<std::uint8_t> build_add_map_action(const Mapping& mapping, const std::string& ip);
	void do_port_mapping(const Mapping& mapping, HTTPTransport& transport);

	// todo, HTTPResponse
	void on_response(const HTTPHeader& header, std::span<const char> buffer,
	                 ActionRequest request);
	void on_connection_error(const boost::system::error_code& ec, ActionRequest request);

	std::string build_http_request(const HTTPRequest& request);
	std::string build_soap_request(const UPnPActionArgs& args);

public:
	IGDevice(boost::asio::io_context& ctx_, const std::string& location, std::string service);
	IGDevice(IGDevice&) = default;
	IGDevice(IGDevice&&) = default;
	IGDevice& operator=(IGDevice&) = default;
	IGDevice& operator=(IGDevice&&) = default;

	void map_port(const Mapping& mapping);
	void unmap_port(std::uint16_t external_port);

	const std::string& host() const;
};

} // upnp, ports, ember