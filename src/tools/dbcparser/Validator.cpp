/*
 * Copyright (c) 2014 - 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Validator.h"
#include "TypeUtils.h"
#include <logger/Logging.h>
#include <limits>
#include <iostream>
#include <typeinfo>
#include <cstdint>

namespace ember { namespace dbc {

/*
 * Searches DBC definitions for the given foreign key. 
 * Only child structs of the root node are considered for matches.
 */
boost::optional<const types::Field*> Validator::locate_fk_parent(const std::string& parent) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	for(auto& def : *definitions_) {
		if(def->name != parent) { //!= for the sake of one less layer of nesting
			continue;
		}

		if(def->type != types::STRUCT) {
			continue;
		}
			
		auto def_s = static_cast<types::Struct*>(def.get());

		for(auto& field : def_s->fields) {
			for(auto& key : field.keys) {
				if(key.type == "primary") {
					return boost::optional<const types::Field*>(&field);
				}
			}
		}
	}

	return boost::none;
}

void Validator::check_foreign_keys(const types::Field& field) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	for(auto& key : field.keys) {
		if(key.type == "foreign") {
			boost::optional<const types::Field*> pk = locate_fk_parent(key.parent);
			auto components(extract_components(field.underlying_type));

			if(!pk) {
				throw exception(field.name + " references a primary key in "
				                + key.parent + " that does not exist");
			}

			if(!key.ignore_type_mismatch && (*pk)->underlying_type != components.first) {
				throw exception(":" + field.name + " => "+ key.parent +
				                " types do not match. Expected " + components.first +
				                ", found " + pk.get()->underlying_type);
			}
		}
	}
}

/*
 * Does basic checking to ensure that each type name is unique and that each field/option
 * within a type is unique to that type.
 *
 * It'd be smarter to do a check on the type tree for name collisions at each depth level
 * rather than doing it here but it's a basic DBC parser and DBCs don't need this sort of
 * thing.
*/
void Validator::check_multiple_definitions(const types::Base* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	if(std::find(names_.begin(), names_.end(), def->name) == names_.end()) {
		names_.emplace_back(def->name);

		if(!def->alias.empty()) {
			names_.emplace_back(def->alias);
		}
	} else {
		throw exception("Multiple definitions of " + def->name + " or its alias found");
	}

	std::vector<std::string> sym_names;

	if(def->type == types::STRUCT) {
		auto def_s = static_cast<const types::Struct*>(def);

		for(auto& symbol : def_s->children) {
			if(std::find(sym_names.begin(), sym_names.end(), symbol->name) == sym_names.end()) {
				sym_names.emplace_back(symbol->name);
			} else {
				throw exception("Multiple definitions of " + symbol->name);
			}
		}
	} else if(def->type == types::ENUM) {
		auto def_s = static_cast<const types::Enum*>(def);

		for(auto& symbol : def_s->options) {
			if(std::find(sym_names.begin(), sym_names.end(), symbol.first) == sym_names.end()) {
				sym_names.emplace_back(symbol.first);
			} else {
				throw exception("Multiple definitions of " + symbol.first);
			}
		}
	} else {
		throw exception("Encountered an unknown type");
	}
}

void Validator::check_key_types(const types::Field& field) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	for(auto& key : field.keys) {
		if(key.type != "primary" && key.type != "foreign") {
			if(key.type.empty()) {
				throw exception(field.name + " did not specify a key type");
			}

			throw exception(key.type + " is not a valid key type" + " for " + field.name);
		} else if(key.type == "foreign" && key.parent.empty()) {
			throw exception(field.name + " - orphaned foreign key");
		} else if(key.type == "primary" && !key.parent.empty()) {
			throw exception(field.name + " - primary key cannot have a parent");
		}
	}
}

void Validator::check_dup_key_types(const types::Struct* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	bool has_primary = false;

	for(auto& field : def->fields) {
		bool has_foreign = false;

		for(auto& key : field.keys) {
			if(key.type == "primary") {
				if(has_primary) {
					throw exception(field.name + " - cannot have multiple primary keys");
				}

				has_primary = true;
			}

			if(key.type == "foreign") {
				if(has_foreign) {
					throw exception(field.name + " - cannot have multiple foreign keys in a single field");
				}

				has_foreign = true;
			}
		}
	}
}

void Validator::add_user_type(TreeNode<std::string>* node, const std::string& type) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	if(std::find_if(node->children.begin(), node->children.end(),
		[&type](const std::unique_ptr<TreeNode<std::string>>& i) {
			return i->t == type;
		}) != node->children.end()) {

		throw exception("Multiple definitions of user-defined type: " + type); 
	}

	node->t = type;
}

void Validator::map_struct_types(TreeNode<std::string>* parent, const types::Struct* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	name_check_(def->name);
	add_user_type(parent, def->name);

	for(auto& child : def->children) {
		auto node = std::make_unique<TreeNode<std::string>>();

		switch(child->type) {
			case types::STRUCT:
				map_struct_types(node.get(), static_cast<types::Struct*>(child.get()));
				break;
			case types::ENUM:
				add_user_type(node.get(), static_cast<types::Enum*>(child.get())->name);
				break;
			default:
				throw exception("Unhandled type");
		}

		node->parent = parent;
		parent->children.emplace_back(std::move(node));
	}	
}

void Validator::recursive_type_parse(TreeNode<std::string>* parent, const types::Base* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	switch(def->type) {
		case types::STRUCT:
			map_struct_types(parent, static_cast<const types::Struct*>(def));
			break;
		case types::ENUM:
			add_user_type(parent, def->name);
			break;
		default:
			throw exception("Unhandled type");
	}
}

/*
 * Recursively searches the type tree for the given type. Only types that are defined
 * before the initial node will be found. This is to help ensure the generator does not
 * attempt to produce code that references 'complete' types before they have been defined.
*/
bool Validator::recursive_ascent_field_type_check(const std::string& type,
                                                  const TreeNode<std::string>* node,
                                                  const TreeNode<std::string>* prev_node) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	// no parent found, must be trying to ascend from the root node
	if(!node) {
		return false;
	}

	// scan children of the current node
	for(auto& child : node->children) {
		// abort searching this level if we come across the node we backed out of
		if(child.get() == prev_node) {
			break;
		}

		if(child->t == type) {
			return true; // found a type match
		}
	}

	// go up a level, if this node has a parent
	return recursive_ascent_field_type_check(type, node->parent, node);
}

/*
 * Checks to see whether the given field is of a valid type. Valid 
 * types are considered to be any that are children of the type tree root
 * (except the one in which the field resides) as well as any that are
 * sibling nodes of the field (same depth, shared parent).
 *
 * This check is pretty naïve but it's not worth refactoring everything and
 * increasing the complexity to improve it.
 */
void Validator::check_field_types(const types::Struct* def, const TreeNode<std::string>* curr_def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	for(auto& field : def->fields) {
		auto components = extract_components(field.underlying_type);

		// check to see whether type is an in-built type
		if(type_map.find(components.first) != type_map.end()) {
			continue;
		}

		// check the type tree
		if(recursive_ascent_field_type_check(components.first, curr_def)) {
			continue;
		}

		throw exception(components.first + " is not a recognised type. "
		                "Ensure the type is defined before its use.");
	}
}

const TreeNode<std::string>* Validator::locate_type_node(const std::string& name,
                                                         const TreeNode<std::string>* node) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	auto it = std::find_if(node->children.begin(), node->children.end(),
		[&name](const std::unique_ptr<TreeNode<std::string>>& i) {
			return i->t == name;
		});

	if(it == node->children.end()) {
		throw exception("Unable to locate type in hierarchy: " + name);
	}

	return it->get();
}

void Validator::validate_struct(const types::Struct* def, const TreeNode<std::string>* types) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	auto node = locate_type_node(def->name, types);

	check_multiple_definitions(def);
	name_check_(def->name);
	
	if(!def->alias.empty()) {
		name_check_(def->alias);
	}

	check_dup_key_types(def);
	check_field_types(def, node);

	for(auto& field : def->fields) {
		if(!field.keys.empty() && !def->dbc) {
			throw exception("Only DBC nodes may contain keys");
		}

		name_check_(field.name);
		check_key_types(field);
		check_foreign_keys(field);
	}

	for(auto& child : def->children) {
		switch(child->type) {
			case types::STRUCT:
				validate_struct(static_cast<const types::Struct*>(child.get()), node);
				break;
			case types::ENUM:
				validate_enum(static_cast<const types::Enum*>(child.get()));
				break;
			default:
				throw exception("Unhandled type");
				break;
		}
	}
}

template<typename T>
void Validator::range_check(long long value) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;
	if(value < std::numeric_limits<T>::lowest() || value > std::numeric_limits<T>::max()) {
		throw exception("Enum option value is out of bounds: " + std::to_string(value)
		                 + " is not within the range of " + typeid(T).name());
	}
}

void Validator::validate_enum_option_value(const std::string& type, const std::string& value) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	int base = value.find("0x") == std::string::npos? 10 : 16;
	std::string set = base == 10? "-0123456789" : "0123456789ABCDEFx";

	if(value.find_first_not_of(set) != std::string::npos) {
		throw exception(value + " is not a valid enum option value");
	}
	
	long long parsed = std::stoll(value, 0, base);
	
	if(type == "int8") {
		range_check<std::int8_t>(parsed);
	} else if(type == "uint8") {
		range_check<std::uint8_t>(parsed);
	} else if(type == "int16") {
		range_check<std::int16_t>(parsed);
	} else if(type == "uint16") {
		range_check<std::uint16_t>(parsed);
	} else if(type == "int32") {
		range_check<std::int32_t>(parsed);
	} else if(type == "uint32") {
		range_check<std::uint32_t>(parsed);
	} else {
		throw exception("Unhandled underlying enum type: " + type);
	}
}

void Validator::validate_enum_options(const types::Enum* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	std::map<std::string, std::string> options;
	
	for(auto& option : def->options) {
		name_check_(def->name);
		validate_enum_option_value(def->underlying_type, option.second);

		if(options.find(option.first) != options.end()) {
			throw exception("Multiple definitions of " + option.first + " in " + def->name);
		}

		if(std::find_if(options.begin(), options.end(),
			[option](std::pair<std::string, std::string> i) {
				return i.second == option.second;
		}) != options.end()) {
			LOG_DEBUG_GLOB << "Duplicate index found for " << option.first << " in " << def->name
			               << ": " << option.second << LOG_ASYNC;
		}

		options[option.first] = option.second;
	}
}

void Validator::validate_enum(const types::Enum* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;
	validate_enum_options(def);
}

void Validator::validate_definition(const types::Base* def) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;
	LOG_DEBUG_GLOB << "Validating " << def->name << LOG_ASYNC;

	switch(def->type) {
		case types::STRUCT:
			validate_struct(static_cast<const types::Struct*>(def), &root_);
			break;
		case types::ENUM:
			validate_enum(static_cast<const types::Enum*>(def));
			break;
		default:
			throw exception("Unhandled type");
			break;
	}
}

/*
 * Loops over the parsed definition vectors and generates an incredibly crude type
 * tree, consisting of any user-defined types (structs and enums). 
 *
 * The type tree is later used to figure out whether field types that reference
 * user-defined types are valid. The type tree begins with a root node.
*/
void Validator::build_type_tree() {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	// todo - remove this bit with VS2015 move
	root_.parent = nullptr;
	root_.t = "_ROOT_";

	for(auto& def : *definitions_) {
		auto node = std::make_unique<TreeNode<std::string>>();
		node->parent = &root_;
		recursive_type_parse(node.get(), def.get());
		root_.children.emplace_back(std::move(node));
	}
}

void Validator::print_type_tree(const TreeNode<std::string>* types, std::size_t depth) {
	std::cout << std::string(depth * 2, '-') << types->t << std::endl;

	for(auto& children : types->children) {
		print_type_tree(children.get(), depth + 1);
	}
}

void Validator::validate(const types::Definitions& definitions) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	definitions_ = &definitions;

	build_type_tree();

	for(auto& def : *definitions_) {
		try { //msvc can't handle try/catch blocks inside range-for without nesting
			validate_definition(def.get());
		} catch(std::exception& e) {
			throw exception(def->name + ": " + e.what());
		}
	}
}

}} //dbc, ember