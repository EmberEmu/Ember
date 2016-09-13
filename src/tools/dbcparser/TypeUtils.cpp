/*
 * Copyright (c) 2014, 2016 Ember
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

const std::unordered_map<std::string, int> type_size_map {
	{ "int8",           1 },
	{ "uint8",          1 },
	{ "int16",          2 },
	{ "uint16",         2 },
	{ "int32",          4 },
	{ "uint32",         4 },
	{ "bool",           1 },
	{ "bool32",         4 },
	{ "string_ref",     4 },
	{ "string_ref_loc", 36 },
	{ "float",          4 },
	{ "double",         8 }
};

const std::map<std::string, std::pair<std::string, bool>> type_map {
	 //type              //real type         //valid enum type?
	{ "int8",           { "std::int8_t",     true  }},
	{ "uint8",          { "std::uint8_t",    true  }},
	{ "int16",          { "std::int16_t",    true  }},
	{ "uint16",         { "std::uint16_t",   true  }},
	{ "int32",          { "std::int32_t",    true  }},
	{ "uint32",         { "std::uint32_t",   true  }},
	{ "bool",           { "bool",            false }},
	{ "bool32",         { "std::uint32_t",   false }},
	{ "string_ref",     { "std::string",     false }},
	{ "string_ref_loc", { "StringRefLoc",    false }},
	{ "float",          { "float",           false }},
	{ "double",         { "double",          false }}
};

const std::unordered_set<std::string> cpp_keywords {
	"alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor"
	"bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
	"compl", "const", "constexpr", "const_cast", "continue", "decltype", "default",
	"delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
	"extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
	"mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator",
	"or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
	"return", "short", "signed", "sizeof", "static", "static_assert", "static_cast",
	"struct", "switch", "template", "this", "thread_local", "throw", "true", "try",
	"typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
	"volatile", "wchar_t", "while", "xor", "xor_eq"
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
 * todo: Should replace this with regex - it doesn't handle names such as AreaPOI correctly
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

types::Base* locate_type_base(const types::Struct& base, const std::string& type_name) {
	for(auto& f : base.children) {
		if(f->name == type_name) {
			return f.get();
		}
	}

	if(base.parent == nullptr) {
		return nullptr;
	}

	return locate_type_base(static_cast<types::Struct&>(*base.parent), type_name);
}

}} //dbc, ember