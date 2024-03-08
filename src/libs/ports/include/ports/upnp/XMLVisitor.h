/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rapidxml/rapidxml.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>

namespace ember::ports::upnp {

struct DeviceVisitor final {
	std::vector<rapidxml::xml_node<char>*> devices;

	void visit(rapidxml::xml_node<char>* node) {
		auto device = node->first_node("device", 0, false);
		devices.emplace_back(device);

		if(!device) {
			return;
		}

		auto device_list = device->first_node("deviceList", 0, false);

		if(!device_list) {
			return;
		}

		visit(device_list);
	}
};

} // upnp, ports, ember