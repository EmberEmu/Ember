/*
 * Copyright (c) 2014 - 2026 Ember
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
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <regex>
#include <utility>
#include <cstddef>

namespace ember::dbc {

using TypeStore = std::vector<std::string>;

class Validator final {
public:
	enum Options {
		val_default           = 1 << 0,
		val_skip_foreign_keys = 1 << 1
	};

private:
	TreeNode<std::string> root_;
	const types::Definitions* definitions_;
	std::vector<std::string> names_;
	Options options_;

	void validate_definition(const types::Base* def);
	void check_multiple_definitions(const types::Base* def);
	void check_key_types(const types::Field& def);
	void check_foreign_keys(const types::Field& def);
	std::optional<std::reference_wrapper<const types::Field>> locate_fk_parent(const std::string_view parent);

	void build_type_tree();
	void recursive_type_parse(TreeNode<std::string>* parent, const types::Base* def);
	void map_struct_types(TreeNode<std::string>* parent, const types::Struct* def);
	void add_user_type(TreeNode<std::string>* node, const std::string_view type);
	void validate_struct(const types::Struct* def, const TreeNode<std::string>* types);
	void validate_enum(const types::Enum* def);
	void check_dup_key_types(const types::Struct* def);
	void validate_enum_options(const types::Enum* def);
	void validate_enum_option_value(const std::string_view type, const std::string_view value);
	void check_field_types(const types::Struct* def, const TreeNode<std::string>* curr_def);
	bool recursive_ascent_field_type_check(const std::string& type, const TreeNode<std::string>* node,
	                                       const TreeNode<std::string>* prev_node = nullptr);
	const TreeNode<std::string>* locate_type_node(const std::string_view name, const TreeNode<std::string>* node);

	template<typename T>
	static void range_check(long long value);

	static void reserved_keyword_test(const std::string_view name);
	static void print_type_tree(const TreeNode<std::string>* types, std::size_t depth = 0);

public:
	Validator()
		: definitions_(nullptr)
		, options_{}{};

	void validate(const types::Definitions& definitions_, Options options = val_default);
};

} // dbc, ember