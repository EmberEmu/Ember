/*
 * Copyright (c) 2014, 2015 Ember
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

struct Struct;
struct Enum;
struct Field;

class TypeVisitor {
public:
	virtual void visit(const types::Struct*) = 0;
	virtual void visit(const types::Enum*) = 0;
	virtual void visit(const types::Field*) = 0;
};

struct Base;

enum Types {
	STRUCT, ENUM
};

typedef std::vector<std::unique_ptr<Base>> Definitions;

struct Key {
	std::string type;
	std::string parent;
	bool ignore_type_mismatch = false;
};

struct IVisitor {
	virtual void accept(TypeVisitor* visitor) = 0;
};

struct Field : IVisitor {
	std::string underlying_type;
	std::string name;
	std::string comment;
	std::vector<Key> keys;

	virtual void accept(TypeVisitor* visitor) {
		visitor->visit(this);
	};
};

struct Base : IVisitor {
	explicit Base(Types type_) : type(type_), parent(nullptr) {}
	Types type;
	std::string name;
	std::string alias;
	std::string comment;
	Base* parent;
};

struct Enum : Base {
	Enum() : Base(ENUM) {}
	std::string underlying_type;
	std::vector<std::pair<std::string, std::string>> options;

	virtual void accept(TypeVisitor* visitor) {
		visitor->visit(this);
	};
};

struct Struct : Base {
	Struct() : Base(STRUCT), dbc(false) {}
	std::vector<Field> fields;
	std::vector<std::unique_ptr<Base>> children;
	bool dbc;

	virtual void accept(TypeVisitor* visitor) {
		visitor->visit(this);
	};

	/* for msvc again - todo, remove in VS2015 */
	void move_op(Struct& src) {
		children = std::move(src.children);
		alias = std::move(src.alias);
		comment = std::move(src.comment);
		fields = std::move(src.fields);
		name = std::move(src.name);
		dbc = src.dbc;
		type = src.type;
	}

	Struct(Struct&& src) : Base(STRUCT) {
		move_op(src);
	}

	Struct& operator=(Struct&& src) {
		move_op(src);
		return *this;
	}

	Struct(Struct& src) = delete;
	Struct& operator=(Struct& src) = delete;
};

}}} //types, dbc, ember