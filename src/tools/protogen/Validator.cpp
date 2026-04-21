/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Validator.h"
#include "TypeUtils.h"
#include <format>
#include <stdexcept>
#include <vector>

namespace ember::protogen {

struct ScopeEntry {
	std::string name;
	std::string type;
};

using Scope = std::vector<ScopeEntry>;

void validate_condition(const jsoncons::json& cond, const Scope& scope,
                        const TypeRegistry& types, std::string_view context);


const ScopeEntry* lookup(const Scope& scope, std::string_view name) {
	for(const auto& e : scope) {
		if(e.name == name) {
			return &e;
		}
	}

	return nullptr;
}

void validate_array(const jsoncons::json& array, const std::string& field_name,
                    const Scope& scope, std::string_view context) {
	if(!array.contains("count_field")) {
		return;
	}
	const auto count_field = array["count_field"].as<std::string>();
	const auto* entry = lookup(scope, count_field);

	if(!entry) {
		throw std::runtime_error(std::format(
			"{}: array '{}' count_field references unknown field '{}'",
			context, field_name, count_field
		));
	}

	if(!is_integral(entry->type)) {
		throw std::runtime_error(std::format(
			"{}: array '{}' count_field '{}' must be an integer primitive, got '{}'",
			context, field_name, count_field, entry->type
		));
	}
}

void validate_fields(const jsoncons::json& fields, const TypeRegistry& types,
                     Scope& scope, std::string_view context) {
	for(const auto& field : fields.array_range()) {
		const auto type = field["type"].as<std::string>();

		if(type == "group") {
			if(field.contains("when")) {
				validate_condition(field["when"], scope, types, context);
			}

			validate_fields(field["fields"], types, scope, context);
			continue;
		}

		if(!is_primitive(type) && !types.find(type)) {
			throw std::runtime_error(std::format(
				"{}: field '{}' references unknown type '{}'",
				context, field["name"].as<std::string>(), type
			));
		}

		if(field.contains("when")) {
			validate_condition(field["when"], scope, types, context);
		}

		if(field.contains("array")) {
			validate_array(field["array"], field["name"].as<std::string>(), scope, context);
		}

		scope.push_back({field["name"].as<std::string>(), type});
	}
}

void check_named_value(const jsoncons::json& value, const ScopeEntry& entry,
                       const TypeRegistry& types, std::string_view context) {
	if(!value.is_string()) {
		return;
	}

	const auto name = value.as<std::string>();
	const auto* def = types.find(entry.type);

	if(!def) {
		throw std::runtime_error(std::format(
			"{}: named value '{}' requires field '{}' to be an enum or flags type, but '{}' is primitive",
			context, name, entry.name, entry.type
		));
	}

	const auto kind = (*def)["kind"].as<std::string>();

	if(kind != "enum" && kind != "flags") {
		throw std::runtime_error(std::format(
			"{}: named value '{}' requires field '{}' to be an enum or flags type, got '{}'",
			context, name, entry.name, kind
		));
	}

	if(!(*def)["values"].contains(name)) {
		throw std::runtime_error(std::format(
			"{}: '{}' is not a declared value of type '{}'",
			context, name, entry.type
		));
	}
}

void validate_condition(const jsoncons::json& cond, const Scope& scope,
                        const TypeRegistry& types, std::string_view context) {
	const auto op = cond["op"].as<std::string>();
	const auto field_name = cond["field"].as<std::string>();
	const auto* entry = lookup(scope, field_name);

	if(!entry) {
		throw std::runtime_error(std::format(
			"{}: condition references unknown field '{}'", context, field_name
		));
	}

	if(!is_primitive(entry->type)) {
		const auto* def = types.find(entry->type);
		const auto kind = def ? (*def)["kind"].as<std::string>() : std::string();

		if(kind != "enum" && kind != "flags") {
			throw std::runtime_error(std::format(
				"{}: field '{}' of type '{}' can't be used in a condition",
				context, field_name, entry->type
			));
		}
	}

	const auto& value = cond["value"];

	if(op == "has_flag") {
		const auto* def = types.find(entry->type);
		if(!def || (*def)["kind"].as<std::string>() != "flags") {
			throw std::runtime_error(std::format(
				"{}: 'has_flag' requires field '{}' to be a flags type, got '{}'",
				context, field_name, entry->type
			));
		}
		check_named_value(value, *entry, types, context);
	} else if(op == "eq") {
		check_named_value(value, *entry, types, context);
	} else if(op == "in") {
		for(const auto& v : value.array_range()) {
			check_named_value(v, *entry, types, context);
		}
	}
}

const jsoncons::json* TypeRegistry::find(const std::string& name) const {
	if(auto it = defs.find(name); it != defs.end()) {
		return &it->second;
	}

	return nullptr;
}

TypeRegistry build_registry(const jsoncons::json& types_doc) {
	TypeRegistry reg;

	for(const auto& pair : types_doc["types"].object_range()) {
		reg.defs.emplace(pair.key(), pair.value());
	}

	return reg;
}

void validate_registry_internals(const TypeRegistry& reg) {
	for(const auto& [name, def] : reg.defs) {
		const auto kind = def["kind"].as<std::string>();

		if(kind != "struct") {
			continue;
		}

		Scope scope;
		validate_fields(def["fields"], reg, scope, std::format("type '{}'", name));
	}
}

void validate_message_fields(const jsoncons::json& fields, const TypeRegistry& reg, std::string_view context) {
	Scope scope;
	validate_fields(fields, reg, scope, context);
}

} // protogen, ember
