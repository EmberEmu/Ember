/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Validator.h"
#include "Scope.h"
#include "TypeUtils.h"
#include <format>
#include <stdexcept>
#include <vector>

namespace ember::protogen {

using Scope = std::vector<ScopeEntry>;

void validate_condition(const jsoncons::json& cond, const Scope& scope,
                        const TypeRegistry& types, std::string_view context);

void validate_array(const jsoncons::json& array, const std::string& /*field_type*/,
                    const std::string& field_name, const Scope& scope,
                    const TypeRegistry& /*types*/, std::string_view context) {
	// Fixed-size arrays of any kind (primitive, enum/flags, struct, string)
	// are allowed — the generator emits an element-by-element loop for the
	// kinds that don't have a whole-array stream overload.
	if(array.contains("size") || array.contains("until_end")) {
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

void validate_as_cast(const jsoncons::json& field, const TypeRegistry& types,
                      std::string_view context) {
	if(!field.contains("as")) {
		return;
	}

	const auto wire = field["type"].as<std::string>();
	const auto as = field["as"].as<std::string>();
	const auto name = field["name"].as<std::string>();

	if(!is_primitive(wire)) {
		throw std::runtime_error(std::format(
			"{}: field '{}' has 'as'='{}' but wire type '{}' isn't primitive; "
			"'as' only applies when the wire type is a bare primitive",
			context, name, as, wire
		));
	}

	const auto* def = types.find(as);

	if(!def) {
		throw std::runtime_error(std::format(
			"{}: field '{}' declares 'as'='{}' which isn't a declared type",
			context, name, as
		));
	}

	const auto kind = (*def)["kind"].as<std::string>();

	if(is_enum_kind(kind) || kind == "flags") {
		const auto declared = (*def)["underlying"].as<std::string>();
		// The cast is sound only when the enum/flag and wire types have
		// the same size and signedness — otherwise reinterpreting bytes
		// between them would silently truncate or widen.
		if(declared != wire) {
			throw std::runtime_error(std::format(
				"{}: field '{}' 'as'='{}' has underlying '{}' but wire type is '{}'; "
				"the two must match for the cast to be sound",
				context, name, as, declared, wire
			));
		}
		return;
	}

	if(kind == "external" && def->contains("underlying")) {
		// An integral-convertible external (typically a DBC inner enum)
		// opts in to `as` casting by declaring `underlying`. Any primitive
		// wire is allowed — the generator emits an explicit static_cast,
		// so a size change is intentional and visible at the call site.
		return;
	}

	throw std::runtime_error(std::format(
		"{}: field '{}' 'as'='{}' resolves to kind '{}'; only enum / flags or external-with-underlying are supported",
		context, name, as, kind
	));
}

void validate_fields(const jsoncons::json& fields, const TypeRegistry& types,
                     Scope& scope, std::string_view context) {
	for(const auto& field : fields.array_range()) {
		const auto type = field["type"].as<std::string>();

		if(type == "group") {
			if(field.contains("when")) {
				validate_condition(field["when"], scope, types, context);
			}

			// group-local fields must not leak into the outer scope: they're
			// only on the wire when the group's 'when' evaluates true, so any
			// reference to them from a sibling/following field would be unsafe
			const auto outer_size = scope.size();
			validate_fields(field["fields"], types, scope, context);
			scope.resize(outer_size);
			continue;
		}

		if(type == "if_chain") {
			// Each branch is an independent conditional scope — like a
			// sequence of groups — so their fields must not leak either.
			for(const auto& branch : field["branches"].array_range()) {
				validate_condition(branch["when"], scope, types, context);
				const auto outer_size = scope.size();
				validate_fields(branch["fields"], types, scope, context);
				scope.resize(outer_size);
			}

			if(field.contains("else")) {
				const auto outer_size = scope.size();
				validate_fields(field["else"], types, scope, context);
				scope.resize(outer_size);
			}

			continue;
		}

		if(!is_primitive(type) && !types.find(type)) {
			throw std::runtime_error(std::format(
				"{}: field '{}' references unknown type '{}'",
				context, field["name"].as<std::string>(), type
			));
		}

		validate_as_cast(field, types, context);

		if(field.contains("when")) {
			validate_condition(field["when"], scope, types, context);
		}

		if(field.contains("array")) {
			validate_array(
				field["array"], type, field["name"].as<std::string>(),
				scope, types, context
			);
		}

		// The scope entry records the semantic type:
		//   - `as` cast wins when present (the field is exposed as an
		//     enum/flags for condition checks).
		//   - An external with `underlying` but no `as` is an alias for
		//     that primitive — the scope reports the primitive so conditions
		//     against the field validate against the integer domain.
		//   - Otherwise the declared type stands as-is.
		auto semantic_type = type;
		if(field.contains("as")) {
			semantic_type = field["as"].as<std::string>();
		} else if(const auto* def = types.find(type);
		          def && (*def)["kind"].as<std::string>() == "external"
		          && def->contains("underlying")) {
			semantic_type = (*def)["underlying"].as<std::string>();
		}
		scope.push_back({field["name"].as<std::string>(), semantic_type});
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
		if(is_primitive(entry.type)) {
			throw std::runtime_error(std::format(
				"{}: named value '{}' requires field '{}' to be an enum or flags type, but '{}' is primitive",
				context, name, entry.name, entry.type
			));
		}

		throw std::runtime_error(std::format(
			"{}: named value '{}' refers to field '{}' of unknown type '{}'",
			context, name, entry.name, entry.type
		));
	}

	const auto kind = (*def)["kind"].as<std::string>();

	if(!is_enum_kind(kind) && kind != "flags") {
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

void check_value(const jsoncons::json& value, const ScopeEntry& entry, const Scope& scope,
                 const TypeRegistry& types, std::string_view context) {
	if(!value.is_object()) {
		check_named_value(value, entry, types, context);
		return;
	}

	const auto ref_name = value["field"].as<std::string>();
	const auto* ref = lookup(scope, ref_name);

	if(!ref) {
		throw std::runtime_error(std::format(
			"{}: condition value references unknown field '{}'", context, ref_name
		));
	}

	if(!is_primitive(ref->type)) {
		const auto* def = types.find(ref->type);
		const auto kind = def? (*def)["kind"].as<std::string>() : std::string();

		if(!is_enum_kind(kind) && kind != "flags") {
			throw std::runtime_error(std::format(
				"{}: value references field '{}' of type '{}' which can't be used in a condition",
				context, ref_name, ref->type
			));
		}
	}
}

void reject_nested_stream_check(const jsoncons::json& cond, std::string_view outer_op,
                                std::string_view context) {
	if(cond["op"].as<std::string>() == "stream_not_empty") {
		throw std::runtime_error(std::format(
			"{}: 'stream_not_empty' cannot be nested inside '{}' — it must be the top-level condition",
			context, outer_op
		));
	}
}

void validate_condition(const jsoncons::json& cond, const Scope& scope,
                        const TypeRegistry& types, std::string_view context) {
	const auto op = cond["op"].as<std::string>();

	// stream state condition: this check has to be performed at runtime
	// (i.e. '!stream.empty()')
	if(op == "stream_not_empty") {
		return;
	}

	if(op == "and" || op == "or") {
		for(const auto& sub : cond["conditions"].array_range()) {
			reject_nested_stream_check(sub, op, context);
			validate_condition(sub, scope, types, context);
		}

		return;
	}

	if(op == "not") {
		const auto& inner = cond["condition"];
		reject_nested_stream_check(inner, op, context);
		validate_condition(inner, scope, types, context);
		return;
	}

	const auto field_name = cond["field"].as<std::string>();
	const auto* entry = lookup(scope, field_name);

	if(!entry) {
		throw std::runtime_error(std::format(
			"{}: condition references unknown field '{}'", context, field_name
		));
	}

	if(op == "empty") {
		const auto* def = types.find(entry->type);
		const auto kind = def? (*def)["kind"].as<std::string>() : std::string();

		if(kind != "string") {
			throw std::runtime_error(std::format(
				"{}: 'empty' requires field '{}' to be a string type, got '{}'",
				context, field_name, entry->type
			));
		}

		return;
	}

	if(!is_primitive(entry->type)) {
		const auto* def = types.find(entry->type);
		const auto kind = def? (*def)["kind"].as<std::string>() : std::string();

		if(!is_enum_kind(kind) && kind != "flags") {
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
	} else if(op == "eq" || op == "neq") {
		check_value(value, *entry, scope, types, context);
	} else if(op == "in") {
		for(const auto& v : value.array_range()) {
			check_named_value(v, *entry, types, context);
		}
	} else {
		throw std::runtime_error(std::format("{}: unknown condition op '{}'", context, op));
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
