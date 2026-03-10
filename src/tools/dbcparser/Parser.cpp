/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include <logger/Logger.h>
#include <rapidxml_utils.hpp>
#include <format>
#include <cstddef>
#include <cstring>

namespace rxml = rapidxml;

namespace ember::dbc {

template<typename T>
const std::string_view Parser::value_string_view(const T* node) const {
	return { node->value(), node->value_size()};
}

template<typename T>
const std::string_view Parser::name_string_view(const T* node) const {
	return { node->name(), node->name_size()};
}

types::Key Parser::parse_field_key(rxml::xml_node<>* property) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	types::Key key;
	auto attr = property->first_attribute("ignore-type-mismatch");

	if(attr) {
		if(std::strncmp(attr->value(), "true", attr->value_size()) == 0
		   || std::strncmp(attr->value(), "1", attr->value_size()) == 0) {
			key.ignore_type_mismatch = true;
		} else {
			throw exception(std::format(
				"{} is not a valid attribute value for ignore-type-mismatch", value_string_view(attr)
			));
		}
	}

	for(rxml::xml_node<>* node = property->first_node(); node != 0; node = node->next_sibling()) {
		if(std::strncmp(node->name(), "type", node->name_size()) == 0) {
			key.type = value_string_view(node);
		} else if(std::strncmp(node->name(), "parent", node->name_size()) == 0) {
			key.parent = value_string_view(node);
		} else {
			throw exception(std::format("Unexpected element in <key>: {}", name_string_view(node)));
		}
	}

	return key;
}

void Parser::parse_enum_options(std::vector<std::pair<std::string_view, std::string_view>>& key,
                                rxml::xml_node<>* property) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	for(rxml::xml_node<>* node = property->first_node(); node; node = node->next_sibling()) {
		std::pair<std::string_view, std::string_view> kv;

		if(std::strncmp(node->name(), "option", node->name_size()) != 0) {
			throw exception(std::format("Unexpected node in <options>: {}", name_string_view(node)));
		}

		for(rxml::xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
			if(std::strncmp(attr->name(), "name", attr->name_size()) == 0) {
				kv.first = value_string_view(attr);
			} else if(std::strncmp(attr->name(), "value", attr->name_size()) == 0) {
				kv.second = value_string_view(attr);
			} else {
				throw exception(std::format("Unexpected attribute in <enum>: {}", name_string_view(attr)));
			}
		}

		key.emplace_back(kv);
	}
}

void Parser::parse_enum_node(types::Enum& type, UniqueCheck& check, rxml::xml_node<>* node) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	if(std::strncmp(node->name(), "name", node->name_size()) == 0) {
		assign_unique(type.name, check.name, node);
		return;
	} else if(std::strncmp(node->name(), "type", node->name_size()) == 0) {
		assign_unique(type.underlying_type, check.type, node);
		return;
	} else if(std::strncmp(node->name(), "alias", node->name_size()) == 0) {
		assign_unique(type.alias, check.alias, node);
		return;
	} else if(std::strncmp(node->name(), "options", node->name_size()) == 0) {
		if(check.options) {
			throw exception("Multiple definitions of <options> not allowed");
		}

		parse_enum_options(type.options, node);
		check.options = true;
		return;
	}

	throw exception(std::format("Unexpected node in <enum>: {}", name_string_view(node)));
}

void Parser::parse_struct_node(types::Struct& type, UniqueCheck& check, rxml::xml_node<>* node) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	if(std::strncmp(node->name(), "name", node->name_size()) == 0) {
		assign_unique(type.name, check.name, node);
		return;
	} else if(std::strncmp(node->name(), "alias", node->name_size()) == 0) {
		assign_unique(type.alias, check.alias, node);
		return;
	} else if(std::strncmp(node->name(), "field", node->name_size()) == 0) {
		auto f = parse_field(node, &type);
		type.fields.emplace_back(f);
		return;
	}

	throw exception(std::format(
		"Unexpected node in {}: {}", (type.dbc? "<dbc>" : "<struct>"), name_string_view(node)
	));
}

void Parser::assign_unique(std::string_view& type, bool& exists, rxml::xml_node<>* node) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	if(exists) {
		throw exception(std::format("Multiple definitions of: {}", name_string_view(node)));
	}

	type = value_string_view(node);
	exists = true;
}

void Parser::parse_field_node(types::Field& field, UniqueCheck& check, 
                              rxml::xml_node<>* node) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	if(std::strncmp(node->name(), "name", node->name_size()) == 0) {
		assign_unique(field.name, check.name, node);
		return;
	} else if(std::strncmp(node->name(), "type", node->name_size()) == 0) {
		assign_unique(field.underlying_type, check.type, node);
		return;
	} else if(std::strncmp(node->name(), "key", node->name_size()) == 0) {
		field.keys.emplace_back(parse_field_key(node));
		return;
	}

	throw exception(std::format("Unknown node found in <field>: {}", name_string_view(node)));
}

types::Field Parser::parse_field(rxml::xml_node<>* root, types::Base* parent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	types::Field field;
	field.parent = parent;
	UniqueCheck check{};

	auto attr = root->first_attribute("comment");

	if(attr) {
		field.comment = value_string_view(attr);
	}

	for(rxml::xml_node<>* node = root->first_node(); node; node = node->next_sibling()) {
		parse_field_node(field, check, node);
	}

	if(!check.type || !check.name) {
		throw exception("A <field> must have at least <name> and <type> nodes");
	}

	return field;
}

types::Enum Parser::parse_enum(rxml::xml_node<>* root, types::Base* parent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	types::Enum parsed;
	parsed.parent = parent;

	UniqueCheck check{};

	auto attr = root->parent()->first_attribute("comment");

	if(attr) {
		parsed.comment = value_string_view(attr);
	}

	for(rxml::xml_node<>* node = root; node; node = node->next_sibling()) {
		parse_enum_node(parsed, check, node);
	}

	if(!check.type || !check.name) {
		throw exception("An <enum> must have at least <name> and <type> nodes");
	}

	return parsed;
}

std::unique_ptr<types::Struct> Parser::parse_struct(rxml::xml_node<>* root, bool dbc, int depth, types::Base* parent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	if(depth > MAX_PARSE_DEPTH) {
		throw exception("Struct nesting is too deep");
	}

	auto parsed = std::make_unique<types::Struct>();
	parsed->dbc = dbc;
	parsed->parent = parent;

	UniqueCheck check{};

	auto attr = root->parent()->first_attribute("comment");
	
	if(attr) {
		parsed->comment = value_string_view(attr);
	}

	for(rxml::xml_node<>* node = root; node; node = node->next_sibling()) {
		if(std::strncmp(node->name(), "struct", node->name_size()) == 0) {
			parsed->children.emplace_back(std::move(parse_struct(node->first_node(), false, depth + 1, parsed.get())));
		} else if(std::strncmp(node->name(), "enum", node->name_size()) == 0) {
			parsed->children.emplace_back(
				std::make_unique<types::Enum>(parse_enum(node->first_node(), parsed.get()))
			);
		} else {
			parse_struct_node(*parsed, check, node);
		}
	}

	if(!check.name) {
		throw exception(std::format("A {} must have at least a <name> node", dbc? "<dbc>" : "<struct>"));
	}

	return parsed;
}

types::Definitions Parser::parse_doc_root(rxml::xml_node<>* parent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	types::Definitions definition;

	for(rxml::xml_node<>* node = parent; node; node = node->next_sibling()) {
		if(std::strncmp(node->name(), "struct", node->name_size()) == 0) {
			definition.emplace_back(std::move(parse_struct(node->first_node(), false)));
		} else if(std::strncmp(node->name(), "dbc", node->name_size()) == 0) {
			definition.emplace_back(std::move(parse_struct(node->first_node(), true)));
		} else if(std::strncmp(node->name(), "enum", node->name_size()) == 0) {
			definition.emplace_back(
				std::make_unique<types::Enum>(parse_enum(node->first_node()))
			);
		} else {
			throw exception(std::format("Unknown node type, {}", name_string_view(node)));
		}
	}

	return definition;
}

types::Definitions Parser::parse(std::string& document) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	rxml::xml_document<> doc;
	doc.parse<rapidxml::parse_fastest>(document.data());

	auto root = doc.first_node();

	if(!root) {
		throw exception("File appears to be empty");
	}

	return parse_doc_root(root);
}

} // dbc, ember