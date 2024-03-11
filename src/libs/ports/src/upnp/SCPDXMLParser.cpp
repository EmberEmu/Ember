/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/SCPDXMLParser.h>

namespace ember::ports::upnp {

SCPDXMLParser::SCPDXMLParser(std::string_view xml) : xml_(std::string(xml)) {
	// rapidxml uses a load of stack space, hence the allocation
	parser_ = std::make_unique<rapidxml::xml_document<>>();
	parser_->parse<0>(xml_.data());
}

SCPDXMLParser::SCPDXMLParser(std::string xml) : xml_(std::move(xml)) {
	parser_ = std::make_unique<rapidxml::xml_document<>>();
	parser_->parse<0>(xml_.data());
}

rapidxml::xml_node<char>* SCPDXMLParser::action(std::string_view action) {
	const auto scpd = parser_->first_node("scpd", 0, false);

	if(!scpd) {
		return nullptr;
	}
	
	const auto al_node = scpd->first_node("actionList", 0, false);

	if(!al_node) {
		return nullptr;
	}

	auto action_node = al_node->first_node("action", 0, false);

	if(!action_node) {
		return nullptr;
	}

	do {
		if(auto node = action_node->first_node("name", 0, false)) {
			if(node->value() == action) {
				return action_node;
			}
		}

		action_node = action_node->next_sibling("action", 0, false);
	} while(action_node);

	return nullptr;
}

std::vector<std::string_view> SCPDXMLParser::arguments(std::string_view action_name,
                                                       std::string_view direction) {
	const auto action_node = action(action_name);

	if(!action_node) {
		return {};
	}

	const auto ag_node = action_node->first_node("argumentList", 0, false);

	if(!ag_node) {
		return {};
	}

	std::vector<std::string_view> arguments;

	auto arg_node = ag_node->first_node("argument", 0, false);

	while(arg_node) {
		auto name = arg_node->first_node("name", 0, false);

		if(name && name->value()) {
			auto dir_node = arg_node->first_node("direction", 0, false);

			if(dir_node && dir_node->value() && dir_node->value() == direction) {
				
				arguments.emplace_back(name->value());
			}
		}		

		arg_node = arg_node->next_sibling("argument", 0, false);
	}
	
	return arguments;
}

} // upnp, ports, ember