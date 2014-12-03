/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DBCParser.h"
#include "Field.h"
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include <iostream>
#include <fstream>

namespace rxml = rapidxml;

namespace ember { namespace dbc {

void Parser::parse_field_key(Key& key, rxml::xml_node<>* property) {
	for(rxml::xml_node<>* node = property->first_node(); node != 0; node = node->next_sibling()) {
		if(strcmp(node->name(), "type") == 0) {
			key.type = node->value();
		} else if(strcmp(node->name(), "parent") == 0) {
			key.parent = node->value();
		} else {
			throw exception(std::string("Unexpected element in <key>: ") + node->name());
		}
	}
}

void Parser::parse_field_options(std::vector<std::pair<std::string, std::string>>& key,
                                 rxml::xml_node<>* property) {
	for(rxml::xml_node<>* node = property->first_node(); node; node = node->next_sibling()) {
		std::pair<std::string, std::string> kv;

		if(strcmp(node->name(), "option")) {
			throw exception(std::string("Unexpected element in <options>: ") + node->name());
		}

		for(rxml::xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
			if(strcmp(attr->name(), "name") == 0) {
				kv.first = attr->value();
			} else if(strcmp(attr->name(), "value") == 0) {
				kv.second = attr->value();
			} else {
				throw exception(std::string("Unexpected attribute in <enum>: ") + attr->name());
			}
		}

		key.push_back(kv);
	}
}

void Parser::parse_field_property(Field& field, ProgressCheck& checker, rxml::xml_node<>* property) {
	if(strcmp(property->name(), "type") == 0) {
		if(!checker.type) {
			field.type = property->value();
			checker.type = true;
		} else {
			throw exception("Multiple definitions of <type> in <field> not allowed");
		}
	} else if(strcmp(property->name(), "name") == 0) {
		if(!checker.name) {
			field.name = property->value();
			checker.name = true;
		} else {
			throw exception("Multiple definitions of <name> in <field> not allowed");
		}
	} else if(strcmp(property->name(), "key") == 0) {
		if(!checker.key) {
			parse_field_key(field.key, property);
			checker.key = true;
		} else {
			throw exception("Multiple definitions of <key> in <field> not allowed");
		}
	} else if(strcmp(property->name(), "options") == 0) {
		if(!checker.options) {
			parse_field_options(field.options, property);
			checker.options = true;
		} else {
			throw exception("Multiple definitions of <options> in <field> not allowed");
		}
	} else {
		throw exception(std::string("Unexpected element in <field>: ") + property->name());
	}
}

Field Parser::parse_field(const std::string& dbc_name, rxml::xml_node<>* field) {
	Field parsed_field{};
	ProgressCheck checker{};

	for(rxml::xml_node<>* node = field->first_node(); node; node = node->next_sibling()) {
		parse_field_property(parsed_field, checker, node);
	}

	return parsed_field;
}

void Parser::parse(const std::string& path) {
	rxml::file<> definition(path.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(definition.data());

	if(!doc.first_node()) {
		return;
	}

	auto root = doc.first_node();
	const std::string dbc_name = root->name();
	
	if(root->first_attribute("alias")) {
		aliases_.emplace(root->name(), root->first_attribute("alias")->value());
	}

	for(rxml::xml_node<>* node = root->first_node("field"); node; node = node->next_sibling()) {
		definitions_[dbc_name].emplace_back(parse_field(dbc_name, node));
	}
}

void Parser::add_definition(const std::string& path) try {
	parse(path);
} catch(std::exception& e) {
	throw parse_error(path, e.what());
} catch(...) {
	throw parse_error(path, "Unknown exception type");
}

void Parser::add_definition(const std::vector<std::string>& paths) {
	for(auto& p : paths) {
		add_definition(p);
	}
}

}} //dbc, ember