/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/cstring_view.hpp>
#include <rapidxml/rapidxml.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace ember::ports::upnp {

class XMLParser final {
	std::string xml_;
	std::unique_ptr<rapidxml::xml_document<>> parser_;

	rapidxml::xml_node<char>* service_search(std::span<rapidxml::xml_node<char>*> devices,
	                                         const cstring_view service) const;

	rapidxml::xml_node<char>* locate_device(const cstring_view device) const;
	rapidxml::xml_node<char>* locate_service(const cstring_view service) const;

public:
	XMLParser(std::string xml);

	std::optional<const std::string_view> get_node_value(
		const cstring_view service,
	    const cstring_view node_name) const;
};

} // upnp, ports, ember