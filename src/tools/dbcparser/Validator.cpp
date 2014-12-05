/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Validator.h"
#include "Definition.h"
#include "Field.h"
#include "Keywords.h"
#include <regex>

namespace ember { namespace dbc {

struct NameTester {
	NameTester() : regex_("^[A-Za-z_][A-Za-z_0-9]*$") { }

	void operator()(const std::string& name) {
		if(std::find(cpp_keywords.begin(), cpp_keywords.end(), name) != cpp_keywords.end()) {
			throw exception(name + " is a reserved word and cannot be used as an identifier");
		}

		if(!std::regex_search(name, regex_)) {
			throw exception(name + " is not a valid C++ identifier");
		}
	}

private:
	std::regex regex_;
};

boost::optional<const Field*> Validator::locate_fk_parent(const std::string& parent) {
	for(auto& def : definitions_) {
		if(def->dbc_name == parent) {
			for(auto& field : def->fields) {
				if(field.is_key && field.key.type == "primary") {
					return boost::optional<const Field*>(&field);
				}
			}
		}
	}

	return boost::optional<const Field*>();
}

void Validator::check_foreign_keys(const Definition* def) {
	for(auto& field : def->fields) {
		if(field.key.type == "foreign") {
			boost::optional<const Field*> pk = locate_fk_parent(field.key.parent);

			if(!pk) {
				throw exception(def->dbc_name + ":" + field.name + " references a primary key in "
					            + field.key.parent + " that does not exist");
			}

			if(def->dbc_name == "Spell") continue;

			if(!field.key.ignore_type_mismatch && (*pk)->type != field.type) {
				throw exception("Foreign key " + def->dbc_name + ":" + field.name + " => "
					            + field.key.parent + " types do not match. Expected "
					            + field.type + ", found " + pk.get()->type);
			}
		}
	}
}

void Validator::check_naming_conventions() {
	NameTester check;

	for(auto& def : definitions_) {
		try {
			check(def->dbc_name);

			if(!def->alias.empty()) {
				check(def->alias);
			}

			for(auto& field : def->fields) {
				check(field.name);
			}
		} catch(std::exception& e) {
			throw exception(def->dbc_name + ": " + e.what());
		}
	}
}

void Validator::check_multiple_definitions() {
	std::vector<std::string> names;

	for(auto& def : definitions_) {
		if(std::find(names.begin(), names.end(), def->dbc_name) == names.end()) {
			names.emplace_back(def->dbc_name);

			if(!def->alias.empty()) {
				names.emplace_back(def->alias);
			}
		} else {
			throw exception("Multiple definitions of " + def->dbc_name + " or its alias found");
		}

		std::vector<std::string> f_names;

		for(auto& field : def->fields) {
			if(std::find(f_names.begin(), f_names.end(), field.name) == f_names.end()) {
				f_names.emplace_back(field.name);
			} else {
				throw exception(def->dbc_name + ": Multiple definitions of " + field.name);
			}
		}
	}
}

void Validator::check_key_types(const Definition* def) {
	for(auto& field : def->fields) {
		if(field.is_key && (field.key.type != "primary" && field.key.type != "foreign")) {
			throw exception(def->dbc_name + ": " + field.key.type + " is not a valid key type");
		}
	}
}

void Validator::validate() {
	check_multiple_definitions();
	check_naming_conventions();

	for(auto& def : definitions_) {
		check_key_types(def);
		check_foreign_keys(def);
	}
}

}} //dbc, ember