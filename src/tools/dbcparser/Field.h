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
#include <utility>

namespace ember { namespace dbc {

struct Key {
	std::string type;
	std::string parent;
	bool ignore_type_mismatch;
};

struct Field {
	std::string type;
	std::string name;
	std::string comment;
	bool is_key;
	Key key;
	std::vector<std::pair<std::string, std::string>> options;
};

}}