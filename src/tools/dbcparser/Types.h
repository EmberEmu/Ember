/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace ember::dbc::types {

struct Struct;
struct Enum;
struct Field;
struct Base;

class TypeVisitor {
public:
	virtual void visit(const types::Struct*, const types::Field* parent) = 0;
	virtual void visit(const types::Enum*) = 0;
	virtual void visit(const types::Field*, const types::Base* parent) = 0;

	virtual ~TypeVisitor() = default;
};

enum class Type {
	t_struct,
	t_enum,
	t_field
};

using Definitions = std::vector<std::unique_ptr<Base>>;

struct Key {
	std::string_view type;
	std::string_view parent;
	bool ignore_type_mismatch = false;
};

struct IVisitor {
	virtual void accept(TypeVisitor* visitor) = 0;
};

struct Base : IVisitor {
	explicit Base(Type type_)
		: type(type_)
		, parent(nullptr) {}

	virtual ~Base() = default;

	Type type;
	std::string_view name;
	std::string_view alias;
	std::string_view comment;
	Base* parent;
};

struct Field final : Base {
	Field() : Base(Type::t_field) {}
	std::string_view underlying_type;
	std::vector<Key> keys;

	virtual void accept(TypeVisitor* visitor) override {
		visitor->visit(this, nullptr);
	};
};

struct Enum final : Base {
	Enum() : Base(Type::t_enum) {}
	std::string_view underlying_type;
	std::vector<std::pair<std::string_view, std::string_view>> options;

	virtual void accept(TypeVisitor* visitor) override {
		visitor->visit(this);
	};
};

struct Struct final : Base {
	Struct() : Base(Type::t_struct), dbc(false) {}
	std::vector<Field> fields;
	std::vector<std::unique_ptr<Base>> children;
	bool dbc;

	virtual void accept(TypeVisitor* visitor) override {
		visitor->visit(this, nullptr);
	};
};

} // types, dbc, ember