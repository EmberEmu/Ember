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
#include <vector>

namespace ember::ports::upnp {

class SCPDXMLParser final {
	std::string xml_;
	std::unique_ptr<rapidxml::xml_document<>> parser_;

public:
	SCPDXMLParser(std::string_view xml);
	SCPDXMLParser(std::string xml);

	rapidxml::xml_node<char>* action(std::string_view action);
	std::vector<std::string_view> arguments(std::string_view action, std::string_view direction);
};

} // upnp, ports, ember