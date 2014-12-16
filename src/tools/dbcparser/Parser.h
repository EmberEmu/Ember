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
#include "Definition.h"
#include <rapidxml.hpp>
#include <string>
#include <vector>

namespace ember { namespace dbc {

class Parser {
	struct ProgressCheck {
		bool type;
		bool name;
		bool options;
	};

	Definition parse_definitions(const std::string& path);

	template <typename T>
	T get_parsed_node(const std::string& dbc_name, rapidxml::xml_node<>* node);

	template <typename T>
	void parse_node_properties(T& field, ProgressCheck& check,
	                      rapidxml::xml_node<>* property);

	template <typename T>
	void parse_node(T& field, ProgressCheck& checker, rapidxml::xml_node<>* property);

	void parse_field_properties(Field& field, ProgressCheck& checker,
	                            rapidxml::xml_node<>* property);

	void parse_field_options(std::vector<std::pair<std::string, std::string>>& key,
	                         rapidxml::xml_node<>* property);

	void parse_field_key(std::vector<Key>& keys, rapidxml::xml_node<>* property);

public:
	Definition parse(const std::string& path);
	std::vector<Definition> parse(const std::vector<std::string>& paths);
};

}} //dbc, ember