/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Exception.h"
#include "Types.h"
#include <rapidxml.hpp>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace ember::dbc {

class Parser final {
	static const int MAX_PARSE_DEPTH = 3;

	struct UniqueCheck {
		bool type, name, alias, options;
	};

 	types::Definitions parse_file(const std::string& path);
	types::Definitions parse_doc_root(rapidxml::xml_node<>* node);

	std::unique_ptr<types::Struct> parse_struct(rapidxml::xml_node<>* root, bool dbc, int depth = 0, types::Base* parent = nullptr);
	void parse_struct_node(types::Struct& structure, UniqueCheck& check, rapidxml::xml_node<>* node);
	
	types::Field parse_field(rapidxml::xml_node<>* root, types::Base* parent = nullptr);
	void parse_field_node(types::Field& field, UniqueCheck& check, rapidxml::xml_node<>* node);
	types::Key parse_field_key(rapidxml::xml_node<>* node);

	types::Enum parse_enum(rapidxml::xml_node<>* root, types::Base* parent = nullptr);
	void parse_enum_node(types::Enum& structure, UniqueCheck& check, rapidxml::xml_node<>* node);
	void parse_enum_options(std::vector<std::pair<std::string, std::string>>& key, rapidxml::xml_node<>* node);

	void assign_unique(std::string& type, bool& exists, rapidxml::xml_node<>* node);

public:
	types::Definitions parse(const std::string& path);
	types::Definitions parse(std::span<const std::string> paths);
};

} // dbc, ember