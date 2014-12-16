/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include "Validator.h"
#include "Field.h"
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include <iostream>

namespace rxml = rapidxml;

namespace ember { namespace dbc {

void Parser::parse_field_key(std::vector<Key>& keys, rxml::xml_node<>* property) {
	auto attr = property->first_attribute("ignore-type-mismatch");
	Key key;

	if(attr) {
		if(strcmp(attr->value(), "true") == 0 || strcmp(attr->value(), "1") == 0) {
			key.ignore_type_mismatch = true;
		} else {
			throw exception(std::string(attr->value() + std::string(" is not a valid attribute"
			                " value for ignore-type-mismatch")));
		}
	}

	for(rxml::xml_node<>* node = property->first_node(); node != 0; node = node->next_sibling()) {
		if(strcmp(node->name(), "type") == 0) {
			key.type = node->value();
		} else if(strcmp(node->name(), "parent") == 0) {
			key.parent = node->value();
		} else {
			throw exception(std::string("Unexpected element in <key>: ") + node->name());
		}
	}

	keys.emplace_back(key);
}

void Parser::parse_field_options(std::vector<std::pair<std::string, std::string>>& key,
                                 rxml::xml_node<>* property) {
	for(rxml::xml_node<>* node = property->first_node(); node; node = node->next_sibling()) {
		std::pair<std::string, std::string> kv;

		if(strcmp(node->name(), "option") != 0) {
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

template<>
void Parser::parse_node(Field& field, ProgressCheck& checker, rxml::xml_node<>* property) {
	parse_field_properties(field, checker, property);
}

template<>
void Parser::parse_node(Type& field, ProgressCheck& checker, rxml::xml_node<>* property) {
	parse_node_properties(field, checker, property);
}

void Parser::parse_field_properties(Field& field, ProgressCheck& checker, rxml::xml_node<>* property) {
	if(strcmp(property->name(), "key") == 0) {
		parse_field_key(field.keys, property);
		return;
	}

	parse_node_properties(field, checker, property);
}

template<typename T>
void Parser::parse_node_properties(T& field, ProgressCheck& checker, rxml::xml_node<>* property) {
	if(strcmp(property->name(), "type") == 0) {
		if(!checker.type) {
			field.type = property->value();
			checker.type = true;
			return;
		}

		throw exception("Multiple definitions of <type> not allowed");
	}

	if(strcmp(property->name(), "name") == 0) {
		if(!checker.name) {
			field.name = property->value();
			checker.name = true;
			return;
		}
		
		throw exception("Multiple definitions of <name> not allowed");
	}
	
	if(strcmp(property->name(), "options") == 0) {
		if(!checker.options) {
			parse_field_options(field.options, property);
			checker.options = true;
			return;
		}

		throw exception("Multiple definitions of <options> not allowed");
	}
	
	throw exception(std::string("Unexpected element in field/typedef: ") + property->name());
}

template <typename T>
T Parser::get_parsed_node(const std::string& dbc_name, rxml::xml_node<>* root) {
	T parsed{};
	ProgressCheck checker{};

	auto attr = root->first_attribute("comment");

	if(attr) {
		parsed.comment = attr->value();
	}

	for(rxml::xml_node<>* node = root->first_node(); node; node = node->next_sibling()) {
		parse_node(parsed, checker, node);
	}

	if(!checker.type || !checker.name) {
		throw exception("A field must have at least a name and a type element");
	}

	return parsed;
}

Definition Parser::parse_definitions(const std::string& path) {
	rxml::file<> definition(path.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(definition.data());

	if(!doc.first_node()) {
		throw exception("File appears to be empty");
	}

	Definition parsed_def;

	auto root = doc.first_node();

	parsed_def.dbc_name = root->name();
	
	if(root->first_attribute("alias")) {
		parsed_def.alias = root->first_attribute("alias")->value();
	}

	if(root->first_attribute("comment")) {
		parsed_def.comment = root->first_attribute("comment")->value();
	}

	if(root->first_attribute("field_count_hint")) try {
		parsed_def.field_count_hint = std::stoi(root->first_attribute("field_count_hint")->value());
	} catch(std::exception& e) {
		throw exception(std::string("On parsing field_count_hint: ") + e.what());
	}

	/* Two passes are performed to allow typedefs to be used before they've been defined within
	   the DBC definition XML */
	for(rxml::xml_node<>* node = root->first_node("typedef"); node; node = node->next_sibling("typedef")) {
		parsed_def.types.emplace_back(get_parsed_node<Type>(parsed_def.dbc_name, node));
	}

	for(rxml::xml_node<>* node = root->first_node(); node; node = node->next_sibling()) {
		if(strcmp(node->name(), "field") == 0) {
			parsed_def.fields.emplace_back(get_parsed_node<Field>(parsed_def.dbc_name, node));
		} else if(strcmp(node->name(), "typedef") != 0) {
			throw exception(std::string("Unknown node type, ") + node->name());
		}
	}

	return parsed_def;
}

Definition Parser::parse(const std::string& path) try {
	return parse_definitions(path);
} catch(std::exception& e) {
	throw parse_error(path, e.what());
} catch(...) {
	throw parse_error(path, "Unknown exception type");
}

std::vector<Definition> Parser::parse(const std::vector<std::string>& paths) {
	std::vector<Definition> defs;

	for(auto& path : paths) {
		try {
			defs.emplace_back(parse_definitions(path));
		} catch(std::exception& e) {
			throw parse_error(path, e.what());
		} catch(...) {
			throw parse_error(path, "Unknown exception type");
		}
	}

	Validator validator(defs);
	validator.validate();

	return defs;
}

}} //dbc, ember