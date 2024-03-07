/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/SSDP.h>

namespace ember::ports::upnp {

struct Mapping {
	std::uint16_t external;
	std::uint16_t internal;
	std::uint32_t ttl;
};

struct UPnPActionArgs {
	std::string action;
	std::string service;
	std::vector<std::pair<std::string, std::string>> arguments;
};

class Device {
	Gateway gateway_;

	std::string build_http_request(const HTTPRequest& request);
	std::string build_soap_request(const UPnPActionArgs& args);

public:
	Device(Gateway gateway) : gateway_(gateway) {}

	void map_port(const Mapping& mapping, const Gateway& gw);
	void unmap_port(std::uint16_t external_port, const Gateway& gw);

	void send_map(const Mapping& mapping, const Gateway& gw);
	void send_unmap();
};

} // upnp, ports, ember