/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/XMLParser.h>
#include <ports/upnp/XMLVisitor.h>

namespace ember::ports::upnp {

XMLParser::XMLParser(std::string_view xml) : xml_(std::string(xml)) {
	// rapidxml uses a load of stack space, hence the allocation
	parser_ = std::make_unique<rapidxml::xml_document<>>();
	parser_->parse<0>(xml_.data());
}

XMLParser::XMLParser(std::string xml) : xml_(std::move(xml)) {
	parser_ = std::make_unique<rapidxml::xml_document<>>();
	parser_->parse<0>(xml_.data());
}

rapidxml::xml_node<char>* XMLParser::service_search(std::span<rapidxml::xml_node<char>*> devices,
                                                    const std::string& type) {
	for(auto device : devices) {
		const auto slist = device->first_node("serviceList", 0, false);

		if(!slist) {
			continue;
		}

		auto service = slist->first_node("service", 0, false);

		while(service) {
			if(service->first_node("serviceType")->value() == type) {
				return service;
			}

			service = service->next_sibling("service");
		}
	}

	return nullptr;
}

rapidxml::xml_node<char>* XMLParser::locate_device(const std::string& type) {
	const auto root = parser_->first_node("root");

	if(!root) {
		return nullptr;
	}

	DeviceVisitor visitor;
	visitor.visit(root);

	for(auto& device : visitor.devices) {
		if(auto dtype = device->first_node("deviceType")) {
			if(dtype->value() == type) {
				return dtype;
			}
		}
	}

	return nullptr;
}

rapidxml::xml_node<char>* XMLParser::locate_service(const std::string& type) {
	const auto root = parser_->first_node("root");

	if(!root) {
		return nullptr;
	}

	DeviceVisitor visitor;
	visitor.visit(root);
	return service_search(visitor.devices, type);
}

std::optional<std::string> XMLParser::get_node_value(const std::string& service,
                                                     const std::string& node_name) {
	auto service_node = locate_service(service);

	if(!service_node) {
		return std::nullopt;
	}

	auto node = service_node->first_node(node_name.c_str(), 0, false);

	if(!node  || !node ->value()) {
		return std::nullopt;
	}

	return node->value();
}

} // upnp, ports, ember