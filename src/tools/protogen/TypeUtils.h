/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string_view>
#include <unordered_map>

namespace ember::protogen {

enum class TypeInfo {
	integral,
	floating_point,
	boolean
};

extern const std::unordered_map<std::string_view, std::pair<std::string_view, TypeInfo>> type_map;

bool is_primitive(std::string_view type);
bool is_integral(std::string_view type);

// `enum` and `enum_class` behave identically in the type system — both are
// named integers with a known member set. They only differ in the keyword
// the generator emits (`enum` vs `enum class`).
inline bool is_enum_kind(std::string_view kind) {
	return kind == "enum" || kind == "enum_class";
}

} // protogen, ember