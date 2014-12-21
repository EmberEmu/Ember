/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Exception.h"
#include "Field.h"
#include "Types.h"
#include <rapidxml.hpp>
#include <string>
#include <vector>

namespace ember { namespace dbc {

class Parser {
	struct UniqueCheck {
		bool type;
		bool name;
		bool alias;
		bool options;
	};

 	types::Definition parse_file(const std::string& path);
	types::Definition parse_doc_root(rapidxml::xml_node<>* node);

	types::Struct parse_struct(rapidxml::xml_node<>* root, int depth = 0);
	void parse_struct_node(types::Struct& structure, UniqueCheck& check, rapidxml::xml_node<>* node);
	
	types::Field parse_field(rapidxml::xml_node<>* root);
	void parse_field_node(types::Field& field, UniqueCheck& check, rapidxml::xml_node<>* node);
	void parse_field_key(std::vector<types::Key>& keys, rapidxml::xml_node<>* node);

	types::Enum parse_enum(rapidxml::xml_node<>* root);
	void parse_enum_node(types::Enum& structure, UniqueCheck& check, rapidxml::xml_node<>* node);
	void parse_enum_options(std::vector<std::pair<std::string, std::string>>& key, rapidxml::xml_node<>* node);

	void assign_unique(std::string& type, bool& exists, rapidxml::xml_node<>* node);

public:
	types::Definition parse(const std::string& path);
	std::vector<types::Definition> parse(const std::vector<std::string>& paths);
};

}} //dbc, ember