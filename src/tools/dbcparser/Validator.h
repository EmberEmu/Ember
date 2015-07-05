/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Exception.h"
#include "Types.h"
#include "TypeUtils.h"
#include "TreeNode.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <limits>

namespace ember { namespace dbc {

//todo - move
struct NameTester {
	NameTester() : regex_("^[A-Za-z_][A-Za-z_0-9]*$") { }

	void operator()(const std::string& name) const {
		if(cpp_keywords.find(name) != cpp_keywords.end()) {
			throw exception(name + " is a reserved word and cannot be used as an identifier");
		}

		if(!std::regex_match(name, regex_)) {
			throw exception(name + " is not a valid C++ identifier");
		}
	}

private:
	const std::regex regex_;
};
typedef std::vector<std::string> TypeStore;

class Validator {
	NameTester tester_;
	std::vector<TreeNode<std::string>> types_;

	std::vector<const types::Definition*> definitions_;
	void validate_definition(const types::Base& def);
	void check_naming_conventions(const types::Definition* def, const NameTester& check);
	void check_multiple_definitions(const types::Definition* def, std::vector<std::string>& names);
	void check_key_types(const types::Field& def);
	void check_foreign_keys(const types::Field& def);
	boost::optional<const types::Field*> locate_fk_parent(const std::string& parent);

	//New
	void build_type_tree();
	void recursive_type_parse(TreeNode<std::string>& parent, const types::Base& def);
	void map_struct_types(TreeNode<std::string>& parent, const types::Struct& def);
	void add_user_type(TreeNode<std::string>& node, std::string type);
	void validate_struct(const types::Struct& def, const TreeNode<std::string>& types);
	void validate_enum(const types::Enum& def);
	void check_dup_key_types(const types::Struct& def);
	void validate_enum_options(const types::Enum& def);
	void validate_enum_option_value(const std::string& type, const std::string& value);
	void check_field_types(const types::Struct& def, const TreeNode<std::string>& types);
	const TreeNode<std::string>& locate_type_node(const std::string& name, const TreeNode<std::string>& node);
	template<typename T> void range_check(long long value);

	void Validator::print_type_tree(const TreeNode<std::string>& types, int depth = 0);

public:
	Validator();
	explicit Validator(const std::vector<types::Definition>& definitions) {
		for(auto& def : definitions) {
			definitions_.emplace_back(&def);
		}
	}

	void add_definition(const types::Definition& definition) {
		definitions_.emplace_back(&definition);
	}

	void validate();
};

}} //dbc, ember