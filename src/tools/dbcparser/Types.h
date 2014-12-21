/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace ember { namespace dbc { namespace types {

struct Base;

enum Types {
	STRUCT, ENUM
};

typedef std::vector<std::unique_ptr<Base>> Definition;

struct Key {
	std::string underlying_type;
	std::string parent;
	bool ignore_type_mismatch;
};

struct Field {
	std::string underlying_type;
	std::string name;
	std::string comment;
	std::vector<Key> keys;
};

struct Base {
	Types type;
	std::string name;
	std::string alias;
	std::string comment;
};

struct Enum : Base {
	std::string underlying_type;
	std::vector<std::pair<std::string, std::string>> options;
};

struct Struct : Base {
	std::vector<Field> fields;
	std::vector<std::unique_ptr<Base>> children;

	/* for msvc again */
	Struct() = default;
	Struct(Struct&& src) {
		this->children = std::move(src.children);
	}
};

}}} //types, dbc, ember