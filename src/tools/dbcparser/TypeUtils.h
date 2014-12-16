/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/optional.hpp>
#include <utility>
#include <string>
#include <hash_set>
#include <map>

namespace ember { namespace dbc {

typedef std::pair<std::string, boost::optional<int>> TypeComponents;

TypeComponents extract_components(const std::string& type);
std::string pascal_to_underscore(std::string name);

extern std::hash_set<std::string> types;
extern std::map<std::string, std::string> type_map;

}} //dbc, ember