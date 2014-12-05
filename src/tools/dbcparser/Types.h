/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <hash_set>

namespace ember { namespace dbc {

std::hash_set<std::string> types {
	"int32", "uint32", "int16", "uint16", "int8", "uint8", "bool", "bool32",
	"enum32", "enumu32", "string_ref", "string_ref_loc", "float", "double",
	"enum8", "enumu8"
};

}} //dbc, ember