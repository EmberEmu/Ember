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

std::hash_set<std::string> cpp_keywords {
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

}} //dbc, ember