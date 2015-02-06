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
	std::string type;
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
	Base(Types type_) : type(type_) {}
	Types type;
	std::string name;
	std::string alias;
	std::string comment;
};

struct Enum : Base {
	Enum() : Base(ENUM) {}
	std::string underlying_type;
	std::vector<std::pair<std::string, std::string>> options;
};

struct Struct : Base {
	Struct() : Base(STRUCT) {}
	std::vector<Field> fields;
	std::vector<std::unique_ptr<Base>> children;

	/* for msvc again - todo, remove in VS2015 */
	void move_op(Struct& src) {
		children = std::move(src.children);
		alias = std::move(src.alias);
		comment = std::move(src.comment);
		fields = std::move(src.fields);
		name = std::move(src.name);
		type = src.type;
	}

	Struct(Struct&& src) : Base(STRUCT) {
		move_op(src);
	}

	Struct& operator=(Struct&& src) {
		move_op(src);
		return *this;
	}
};

}}} //types, dbc, ember