/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/XMLParser.h>
#include <ports/upnp/XMLVisitor.h>
#include <cstring>

namespace ember::ports::upnp {

XMLParser::XMLParser(std::string xml) : xml_(std::move(xml)) {
	parser_ = std::make_unique<rapidxml::xml_document<>>();
	parser_->parse<rapidxml::parse_fastest>(xml_.data());
}

rapidxml::xml_node<char>* XMLParser::service_search(std::span<rapidxml::xml_node<char>*> devices,
                                                    const std::string_view type) const {
	for(auto device : devices) {
		const auto slist = device->first_node("serviceList", 0, false);

		if(!slist) {
			continue;
		}

		auto service = slist->first_node("service", 0, false);

		while(service) {
			auto node = service->first_node("serviceType");
			const auto max_count = std::min(type.size(), node->value_size());

			if(std::strncmp(node->value(), type.data(), max_count) == 0) {
				return service;
			}

			service = service->next_sibling("service");
		}
	}

	return nullptr;
}

rapidxml::xml_node<char>* XMLParser::locate_device(const std::string_view device) const {
	const auto root = parser_->first_node("root");

	if(!root) {
		return nullptr;
	}

	DeviceVisitor visitor;
	visitor.visit(root);

	for(auto& found_device : visitor.devices) {
		if(auto dtype = found_device->first_node("deviceType")) {
			const auto max_count = std::min(device.size(), dtype->value_size());

			if(std::strncmp(dtype->value(), device.data(), max_count) == 0) {
				return dtype;
			}
		}
	}

	return nullptr;
}

rapidxml::xml_node<char>* XMLParser::locate_service(const std::string_view service) const {
	const auto root = parser_->first_node("root", 0, false);

	if(!root) {
		return nullptr;
	}

	DeviceVisitor visitor;
	visitor.visit(root);
	return service_search(visitor.devices, service);
}

std::optional<const std::string_view> XMLParser::get_node_value(const std::string_view service,
                                                                const std::string_view node_name) const {
	auto service_node = locate_service(service);

	if(!service_node) {
		return std::nullopt;
	}

	auto node = service_node->first_node(node_name.data(), node_name.size(), false);

	if(!node || !node ->value()) {
		return std::nullopt;
	}

	return std::string_view(node->value(), node->value_size());
}

} // upnp, ports, ember