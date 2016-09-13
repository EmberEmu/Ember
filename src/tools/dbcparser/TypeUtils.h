/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Types.h"
#include <boost/optional.hpp>
#include <utility>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace ember { namespace dbc {

typedef std::pair<std::string, boost::optional<int>> TypeComponents;

TypeComponents extract_components(const std::string& type);
std::string pascal_to_underscore(std::string name);
types::Base* locate_type_base(const types::Struct& base, const std::string& type_name);

const extern std::map<std::string, std::pair<std::string, bool>> type_map;
const extern std::unordered_map<std::string, int> type_size_map;
const extern std::unordered_set<std::string> cpp_keywords;

class TypeMetrics : public types::TypeVisitor {
public:
	unsigned int fields = 0;
	unsigned int record_size = 0;

	void visit(const types::Struct* type) override {
		// we don't care about structs
	}

	void visit(const types::Enum* type) override {
		++fields;
		record_size += type_size_map.at(type->underlying_type);
	}

	void visit(const types::Field* type) override {
		auto components = extract_components(type->underlying_type);
		int scalar_size = type_size_map.at(components.first);

		// handle arrays
		unsigned int additional_fields = 1;

		if(components.first == "string_ref_loc") {
			additional_fields = 9;
		}

		if(components.second) {
			additional_fields *= *components.second;
			record_size += (scalar_size * (*components.second));
		} else {
			record_size += scalar_size;
		}

		fields += additional_fields;
	}
};

template<typename T>
void walk_dbc_fields(const types::Struct* dbc, T& visitor) {
	for(auto f : dbc->fields) {
		// if this is a user-defined struct, we need to go through that type too
		// if it's an enum, we can just grab the underlying type
		auto components = extract_components(f.underlying_type);
		auto it = type_map.find(components.first);

		if(it != type_map.end()) {
			visitor.visit(&f);
		} else {
			auto found = locate_type_base(*dbc, components.first);

			if(!found) {
				throw std::runtime_error("Unknown field type encountered, " + f.underlying_type);
			}

			if(found->type == types::STRUCT) {
				walk_dbc_fields(static_cast<types::Struct*>(found), visitor);
			} else if(found->type == types::ENUM) {
				visitor.visit(static_cast<types::Enum*>(found));
			}
		}
	}
}

}} //dbc, ember