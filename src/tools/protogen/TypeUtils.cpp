/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "TypeUtils.h"

namespace ember::protogen {

using enum TypeInfo;

const std::unordered_map<std::string_view, std::pair<std::string_view, TypeInfo>> type_map {
	// type              // real type
	{ "uint8",          { "std::uint8_t",    integral       }},
	{ "uint16",         { "std::uint16_t",   integral       }},
	{ "uint32",         { "std::uint32_t",   integral       }},
	{ "uint64",         { "std::uint64_t",   integral       }},
	{ "int8",           { "std::int8_t",     integral       }},
	{ "int16",          { "std::int16_t",    integral       }},
	{ "int32",          { "std::int32_t",    integral       }},
	{ "int64",          { "std::int64_t",    integral       }},
	{ "float",          { "float",           floating_point }},
	{ "double",         { "double",          floating_point }},
	{ "bool",           { "bool",            boolean        }},
	{ "bool32",         { "std::uint32_t",   boolean        }},
};

bool is_primitive(std::string_view type) {
	return type_map.contains(type);
}

bool is_integral(std::string_view type) {
	if(auto it = type_map.find(type); it != type_map.end()) {
		return it->second.second == integral;
	}

	return false;
}

} // protogen, ember