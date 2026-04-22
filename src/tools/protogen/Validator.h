/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <jsoncons/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ember::protogen {

struct TypeRegistry {
	std::unordered_map<std::string, jsoncons::json> defs;
	const jsoncons::json* find(const std::string& name) const;
};

TypeRegistry build_registry(const jsoncons::json& types_doc);
void validate_registry_internals(const TypeRegistry& reg);
void validate_message_fields(const jsoncons::json& fields, const TypeRegistry& reg, std::string_view context);

} // protogen, ember
