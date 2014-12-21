/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "TypeUtils.h"
#include "Exception.h"
#include <exception>
#include <regex>
#include <cctype>

namespace ember { namespace dbc {

//todo, temporary home
std::hash_set<std::string> dbc_types {
	"int32", "uint32", "int16", "uint16", "int8", "uint8", "bool", "bool32",
	"enum32", "enumu32", "string_ref", "string_ref_loc", "float", "double",
	"enum8", "enumu8"
};

//todo, temporary home
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

/* 
 * Takes a string such as int[5] and splits it into the type (int)
 * and an optional element count (5).
 */
TypeComponents extract_components(const std::string& type) {
	TypeComponents components;
	std::regex pattern(R"(([^[]+)(?:\[(.*)\])?)");
	std::smatch matches;

	if(std::regex_match(type, matches, pattern)) {
		components.first = matches[1].str();

		if(!matches[2].str().empty()) {
			try {
				components.second = std::stoi(matches[2].str());

				if(components.second < 0) {
					throw std::exception(); //todo
				}
			} catch(std::exception) {
				throw exception(matches[2].str() + " is not a valid array entry count"
					" for " + components.first);
			}
		}
	}

	return components;
}

/*
 * Slightly hacky function for converting PascalCaseNames to lower_case_names that
 * match Ember's naming convention. This function is only any use for ASCII strings.
 * Example: SoundEntries => sound_entries
 * Example: NPCSounds => npc_sounds (handles acronyms)
 */
std::string pascal_to_underscore(std::string name) {
	const std::string uc_set("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	std::size_t found = name.find_first_of(uc_set);
	bool first = true;
	bool next_capitalised = false;

	while(found != std::string::npos) {
		name[found] = std::tolower(name[found]);

		/* Bounds check to ensure we don't go over the string length when looking
		   ahead, although this isn't strictly needed as of C++11 given that
		   string[string.length()] is guaranteed to hold a terminator and
		   that the loop will exit after the final match */
		if(found + 1 < name.length()) {
			char next = name[found + 1];
			next_capitalised = next != std::tolower(name[found + 1]);
		} else {
			next_capitalised = false;
		}

		if(!next_capitalised && !first) {
			name.insert(found, 1, '_');
		}
		
		found = name.find_first_of(uc_set, found + 1);
		first = false;
	}

	return name;
}

}} //dbc, ember