/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Types.h"
#include <string>
#include <hash_set>
#include <map>

namespace ember { namespace dbc {

std::hash_set<std::string> types {
	"int32", "uint32", "int16", "uint16", "int8", "uint8", "bool", "bool32",
	"enum32", "enumu32", "string_ref", "string_ref_loc", "float", "double",
	"enum8", "enumu8"
};

std::map<std::string, std::string> type_map {
		{ "int8", "std::int8_t" },
		{ "uint8", "std::uint8_t" },
		{ "int16", "std::int16_t" },
		{ "uint16", "std::oint16_t" },
		{ "int32", "std::int32_t" },
		{ "uint32", "std::uint32_t" },
		{ "bool", "bool" },
		{ "bool32", "std::uint32_t" },
		{ "enum8", "std::int8_t" },
		{ "enumu8", "std::uint8_t" },
		{ "enum16", "std::int16_t" },
		{ "enumu16", "std::uint16_t" },
		{ "enum32", "std::int32_t" },
		{ "enumu32", "std::uint32_t" },
		{ "string_ref", "std::string" },
		{ "string_ref_loc", "std::string" },
		{ "float", "float" },
		{ "double", "double" },
};

}} //dbc, ember