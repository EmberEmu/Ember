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
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace ember::ports::upnp {

class XMLParser {
	std::string xml_;
	std::unique_ptr<rapidxml::xml_document<>> parser_;

	rapidxml::xml_node<char>* service_search(std::span<rapidxml::xml_node<char>*> devices,
	                                         const std::string& type);

public:
	XMLParser(std::string_view xml);
	XMLParser(std::string xml);

	rapidxml::xml_node<char>* locate_device(const std::string& type);
	rapidxml::xml_node<char>* locate_service(const std::string& type);

	std::optional<std::string> get_node_value(const std::string& service,
	                                          const std::string& node_name);
};

} // upnp, ports, ember