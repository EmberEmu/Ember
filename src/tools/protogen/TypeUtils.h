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
	integral        = 1 << 1,
	enumerator      = 1 << 2,
	floating_point  = 1 << 3,
	boolean         = 1 << 4,
	primitive       = 1 << 5
};

extern const std::unordered_map<std::string_view, std::pair<std::string_view, TypeInfo>> type_map;

bool is_primitive(std::string_view type);
bool is_integral(std::string_view type);

} // protogen, ember