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

enum class Protocol {
	PROTO_TCP, PROTO_UDP
};

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

enum class ActionState {
	IGDD_CACHE, SCPD_CACHE, ACTION
};

using Result = std::function<void(bool)>;

struct ActionRequest {
	using Handler = std::function<void(HTTPTransport&)>;

	std::unique_ptr<HTTPTransport> transport;
	ActionState state = ActionState::IGDD_CACHE;
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

	bool add_port_mapping(Mapping& mapping, HTTPTransport& transport);
	bool delete_port_mapping(Mapping& mapping, HTTPTransport& transport);
	void handle_scpd(const HTTPHeader& header, std::string_view body);
	void handle_igdd(const HTTPHeader& header, const std::string_view xml);
	void parse_location(const std::string& location);
	void fetch_scpd(std::shared_ptr<ActionRequest> request);
	void fetch_device_description(std::shared_ptr<ActionRequest> request);

	void launch_request(std::shared_ptr<ActionRequest> request);
	void execute_request(std::shared_ptr<ActionRequest> request);
	void process_action_result(const HTTPHeader& header, std::shared_ptr<ActionRequest> request);
	void process_request(std::shared_ptr<ActionRequest> request);

	void on_response(const HTTPHeader& header, std::span<const char> buffer,
	                 std::shared_ptr<ActionRequest> request);

	void on_connection_error(const boost::system::error_code& ec,
	                         std::shared_ptr<ActionRequest> request);

	template<typename BufType>
	BufType build_http_post_request(std::string&& body, const std::string& action,
	                                const std::string& control_url);

	template<typename BufType>
	BufType build_http_request(const HTTPRequest& request);

	std::string build_soap_request(const UPnPActionArgs& args);
	std::string build_upnp_add_mapping(const Mapping& mapping);
	std::string build_upnp_del_mapping(const Mapping& mapping);

public:
	IGDevice(boost::asio::io_context& ctx_, std::string bind,
	         std::string service, const std::string& location);

	IGDevice(IGDevice&) = default;
	IGDevice(IGDevice&&) = default;
	IGDevice& operator=(IGDevice&) = default;
	IGDevice& operator=(IGDevice&&) = default;

	void map_port(Mapping mapping, Result cb);
	void unmap_port(Mapping mapping, Result cb);

	const std::string& host() const;
};

} // upnp, ports, ember