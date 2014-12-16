/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Exception.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace ember { namespace dbc {

struct Definition;
struct Field;
struct NameTester;

class Validator {
	std::vector<const Definition*> definitions_;

	void check_types(const Definition* def);
	void check_naming_conventions(const Definition* def, const NameTester& check);
	void check_multiple_definitions(const Definition* def, std::vector<std::string>& names);
	void check_key_types(const Field& def);
	void check_foreign_keys(const Field& def);
	void check_dup_key_types(const Definition* def);
	boost::optional<const Field*> locate_fk_parent(const std::string& parent);

public:
	Validator();
	Validator(const std::vector<Definition>& definitions) {
		for(auto& def : definitions) {
			definitions_.emplace_back(&def);
		}
	}

	void add_definition(const Definition& definition) {
		definitions_.emplace_back(&definition);
	}

	void validate();
};

}} //dbc, ember