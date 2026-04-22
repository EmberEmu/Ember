/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Generator.h"
#include "Scope.h"
#include "TypeUtils.h"
#include <nlohmann/json.hpp>
#include <inja/inja.hpp>
#include <algorithm>
#include <chrono>
#include <format>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ember::protogen {

struct WalkState {
	std::vector<std::string> members;
	std::set<std::string> includes;
	std::vector<std::string> read_ops;
	std::vector<std::string> write_ops;
	int read_tab = 2;
	int write_tab = 2;
};

int current_year() {
	const auto time = std::chrono::system_clock::now();
	const std::chrono::year_month_day date = std::chrono::floor<std::chrono::days>(time);
	return static_cast<int>(date.year());
}

std::string indent(int tab_depth) {
	return std::string(tab_depth, '\t');
}

std::string cpp_type_for_primitive(std::string_view type) {
	if(auto it = type_map.find(type); it != type_map.end()) {
		return std::string(it->second.first);
	}

	throw std::runtime_error(
		std::format("unknown primitive '{}'", type)
	);
}

std::string kind_of(const jsoncons::json& def) {
	return def["kind"].as<std::string>();
}

void add_type_include(const std::string& type, const TypeRegistry& reg, std::set<std::string>& out) {
	if(is_primitive(type)) {
		return;
	}

	const auto* def = reg.find(type);

	if(!def) {
		throw std::runtime_error(std::format(
			"unknown type '{}' reached code generation — should have been rejected by validation", type
		));
	}

	const auto kind = kind_of(*def);

	if(kind == "enum" || kind == "flags" || kind == "struct") {
		out.emplace(std::format("generated/types/{}.h", type));
	} else if(kind == "external") {
		out.emplace((*def)["include"].as<std::string>());
	}
}

std::string header_namespace(const jsoncons::json& def) {
	if(def.contains("cpp_namespace")) {
		return def["cpp_namespace"].as<std::string>();
	}
	return "ember::protocol";
}

std::string cpp_type_name(const std::string& type, const TypeRegistry& reg) {
	if(is_primitive(type)) {
		return cpp_type_for_primitive(type);
	}

	const auto* def = reg.find(type);

	if(!def) {
		throw std::runtime_error(std::format(
			"unknown type '{}' reached code generation — should have been rejected by validation", type
		));
	}

	if(kind_of(*def) == "string") {
		return "std::string";
	}

	if(def->contains("cpp_namespace")) {
		return std::format("{}::{}", (*def)["cpp_namespace"].as<std::string>(), type);
	}

	return type;
}

std::string string_adaptor_expr(const jsoncons::json& type_def, std::string_view expr) {
	const auto encoding = type_def["encoding"].as<std::string>();

	if(encoding == "null_terminated") {
		return std::format("spark::io::null_terminated({})", expr);
	}

	const auto length_type = type_def["length_type"].as<std::string>();
	const auto length_cpp = cpp_type_for_primitive(length_type);

	if(encoding == "prefixed") {
		return std::format("spark::io::prefixed<std::string, {}>({})", length_cpp, expr);
	} 
	
	if(encoding == "prefixed_null_terminated") {
		return std::format("spark::io::prefixed_null_terminated<std::string, {}>({})", length_cpp, expr);
	}

	throw std::runtime_error(
		std::format("unknown string encoding '{}'", encoding)
	);
}

std::string render_condition(const jsoncons::json& cond,
                             const TypeRegistry& reg,
                             std::string_view prefix,
                             std::span<const ScopeEntry> scope) {
	const auto op = cond["op"].as<std::string>();

	if(op == "stream_not_empty") {
		return "!stream.empty()";
	}

	const auto field_name = cond["field"].as<std::string>();
	const auto qualified = std::format("{}{}", prefix, field_name);
	const auto* entry = lookup(scope, field_name);

	if(!entry) {
		throw std::runtime_error(std::format("condition references unknown field '{}'", field_name));
	}

	const auto& type = entry->type;
	const auto& value = cond["value"];
	const auto enum_qualified = cpp_type_name(type, reg);

	auto render_named = [&](const std::string& enumerator) {
		return std::format("{}::{}", enum_qualified, enumerator);
	};

	if(op == "eq") {
		if(value.is_string()) {
			return std::format("{} == {}", qualified, render_named(value.as<std::string>()));
		}

		return std::format("{} == {}", qualified, value.as<std::int64_t>());
	}

	if(op == "in") {
		std::string joined;
		for(const auto& v : value.array_range()) {
			if(!joined.empty()) {
				joined += " || ";
			}

			if(v.is_string()) {
				joined += std::format("{} == {}", qualified, render_named(v.as<std::string>()));
			} else {
				joined += std::format("{} == {}", qualified, v.as<std::int64_t>());
			}
		}

		return joined;
	}

	if(op == "has_flag") {
		if(value.is_string()) {
			return std::format("{} & {}", qualified, render_named(value.as<std::string>()));
		}

		return std::format("{} & {}", qualified, value.as<std::int64_t>());
	}

	throw std::runtime_error(std::format("unknown condition op '{}'", op));
}

std::string cpp_element_type(const std::string& type, const TypeRegistry& reg) {
	return cpp_type_name(type, reg);
}

void walk(const jsoncons::json& fields, const TypeRegistry& reg, WalkState& state,
          std::vector<ScopeEntry>& scope, const std::string& prefix);

// emits stream ops for a single scalar value - writes to both read_ops and write_ops
void emit_scalar_stream_op(const std::string& name, const std::string& type,
                           const TypeRegistry& reg, WalkState& state, const std::string& prefix) {
	const auto qualified = prefix + name;

	if(is_primitive(type)) {
		state.read_ops.emplace_back(std::format("{}stream >> {};", indent(state.read_tab), qualified));
		state.write_ops.emplace_back(std::format("{}stream << {};", indent(state.write_tab), qualified));
		return;
	}

	const auto* def = reg.find(type);

	if(!def) {
		throw std::runtime_error(
			std::format("unknown type '{}' for field '{}'", type, name)
		);
	}

	const auto kind = kind_of(*def);

	if(kind == "enum" || kind == "flags" || kind == "external") {
		state.read_ops.emplace_back(std::format("{}stream >> {};", indent(state.read_tab), qualified));
		state.write_ops.emplace_back(std::format("{}stream << {};", indent(state.write_tab), qualified));
		return;
	}

	if(kind == "string") {
		const auto adaptor = string_adaptor_expr(*def, qualified);
		state.read_ops.emplace_back(std::format("{}stream >> {};", indent(state.read_tab), adaptor));
		state.write_ops.emplace_back(std::format("{}stream << {};", indent(state.write_tab), adaptor));
		state.includes.insert("spark/buffers/StringAdaptors.h");
		return;
	}

	if(kind == "struct") {
		std::vector<ScopeEntry> nested_scope;
		walk((*def)["fields"], reg, state, nested_scope, qualified + ".");
		return;
	}

	throw std::runtime_error(
		std::format("unsupported kind '{}' for field '{}'", kind, name)
	);
}

void emit_stream_op(const jsoncons::json& field, const std::string& type,
                    const TypeRegistry& reg, WalkState& state, const std::string& prefix) {
	const auto name = field["name"].as<std::string>();

	if(!field.contains("array")) {
		emit_scalar_stream_op(name, type, reg, state, prefix);
		return;
	}

	const auto& arr = field["array"];
	const auto qualified = prefix + name;

	// fixed-size arrays - rely on the stream overloads for std::array
	if(arr.contains("size")) {
		state.read_ops.emplace_back(std::format("{}stream >> {};", indent(state.read_tab), qualified));
		state.write_ops.emplace_back(std::format("{}stream << {};", indent(state.write_tab), qualified));
		state.includes.insert("array");
		return;
	}

	// vector with size held in preceding field
	const auto count_field = arr["count_field"].as<std::string>();
	const auto qualified_count = prefix + count_field;
	state.includes.emplace("vector");

	state.read_ops.emplace_back(
		std::format("{}{}.resize({});", indent(state.read_tab), qualified, qualified_count)
	);

	state.read_ops.emplace_back(
		std::format("{}for(auto& e : {}) {{", indent(state.read_tab), qualified)
	);

	state.write_ops.emplace_back(
		std::format("{}for(const auto& e : {}) {{", indent(state.write_tab), qualified)
	);

	++state.read_tab;
	++state.write_tab;
	emit_scalar_stream_op("e", type, reg, state, std::string());
	--state.read_tab;
	--state.write_tab;

	state.read_ops.emplace_back(std::format("{}}}", indent(state.read_tab)));
	state.write_ops.emplace_back(std::format("{}}}", indent(state.write_tab)));
}

std::string member_decl(const jsoncons::json& field, const TypeRegistry& reg) {
	const auto name = field["name"].as<std::string>();
	const auto type = field["type"].as<std::string>();
	const auto elem = cpp_element_type(type, reg);

	if(field.contains("array")) {
		const auto& arr = field["array"];

		if(arr.contains("size")) {
			const auto sz = arr["size"].as<std::int64_t>();
			return std::format("\tstd::array<{}, {}> {}{{}};", elem, sz, name);
		}
		return std::format("\tstd::vector<{}> {};", elem, name);
	}

	return std::format("\t{} {};", elem, name);
}

void walk(const jsoncons::json& fields, const TypeRegistry& reg, WalkState& state,
          std::vector<ScopeEntry>& scope, const std::string& prefix) {
	const bool emit_members = prefix.empty();
	bool first = true;
	bool prev_is_group = false;

	for(const auto& field : fields.array_range()) {
		const auto type = field["type"].as<std::string>();
		const bool current_is_group = (type == "group");
		const bool need_blank = !first && (prev_is_group || current_is_group);

		if(need_blank) {
			state.read_ops.emplace_back("");
			state.write_ops.emplace_back("");
		}

		if(current_is_group) {
			const auto& when = field["when"];
			const auto op = when["op"].as<std::string>();

			// stream_not_empty only applies to reading the packet
			const bool read_only = (op == "stream_not_empty");
			const auto cond = render_condition(when, reg, prefix, scope);

			state.read_ops .emplace_back(
				std::format("{}if({}) {{", indent(state.read_tab), cond)
			);

			if(!read_only) {
				state.write_ops.emplace_back(
					std::format("{}if({}) {{", indent(state.write_tab), cond)
				);
			}

			++state.read_tab;

			if(!read_only) {
				++state.write_tab;
			}

			// group-local field names must not leak past the closing brace,
			// so they can't be referenced by later scope lookups
			// (e.g. in a sibling group's `when` expression)
			const auto outer_size = scope.size();
			walk(field["fields"], reg, state, scope, prefix);
			scope.resize(outer_size);

			--state.read_tab;

			if(!read_only) {
				--state.write_tab;
			}

			state.read_ops .emplace_back(
				std::format("{}}}", indent(state.read_tab))
			);

			if(!read_only) {
				state.write_ops.emplace_back(
					std::format("{}}}", indent(state.write_tab))
				);
			}
		} else {
			scope.emplace_back(field["name"].as<std::string>(), type);
			add_type_include(type, reg, state.includes);

			if(emit_members) {
				state.members.emplace_back(member_decl(field, reg));
				const auto* def = reg.find(type);

				if(def && kind_of(*def) == "string") {
					state.includes.emplace("string");
				}
			}

			emit_stream_op(field, type, reg, state, prefix);
		}

		first = false;
		prev_is_group = current_is_group;
	}
}

std::string join_lines(std::span<const std::string> lines) {
	std::ostringstream out;

	for(std::size_t i = 0; i < lines.size(); ++i) {
		out << lines[i];

		if(i + 1 < lines.size()) {
			out << '\n';
		}
	}

	return out.str();
}

GeneratedFile generate_message(const jsoncons::json& message,
                               const TypeRegistry& registry,
                               const std::filesystem::path& templates_dir) {
	const auto name = message["name"].as<std::string>();
	const auto opcode = message["opcode"].as<std::string>();
	const auto direction = message["direction"].as<std::string>();

	WalkState state;
	std::vector<ScopeEntry> scope;
	walk(message["fields"], registry, state, scope, {});

	nlohmann::json data;
	data["year"] = current_year();
	data["name"] = name;
	data["opcode"] = opcode;
	data["direction"] = direction;
	data["packet_template"] = (direction == "server") ? "ServerPacket" : "ClientPacket";
	data["opcode_enum"] = (direction == "server") ? "ServerOpcode" : "ClientOpcode";
	data["members"] = join_lines(state.members);
	data["read_body"] = join_lines(state.read_ops);
	data["write_body"] = join_lines(state.write_ops);

	auto includes_json = nlohmann::json::array();

	for(const auto& h : state.includes) {
		includes_json.push_back(h);
	}

	data["includes"] = std::move(includes_json);

	inja::Environment env;
	auto tpl = env.parse_template((templates_dir / "Message.h_"));
	auto content = env.render(tpl, data);

	return { std::format("{}/{}.h", direction, name), std::move(content) };
}

std::string generate_aggregator(std::span<const std::string> relative_paths,
                                const std::filesystem::path& templates_dir) {
	auto headers = nlohmann::json::array();

	for(const auto& p : relative_paths) {
		headers.emplace_back(p);
	}

	nlohmann::json data;
	data["headers"] = std::move(headers);
	data["year"] = current_year();

	inja::Environment env;
	const auto tpl = env.parse_template((templates_dir / "Packets.h_"));
	return env.render(tpl, data);
}

void collect_struct_members(const jsoncons::json& fields, const TypeRegistry& reg,
                            nlohmann::json& out, std::set<std::string>& includes) {
	for(const auto& field : fields.array_range()) {
		const auto type = field["type"].as<std::string>();

		if(type == "group") {
			collect_struct_members(field["fields"], reg, out, includes);
			continue;
		}

		const auto elem = cpp_element_type(type, reg);

		std::string decl_type;
		std::string suffix;

		if(field.contains("array")) {
			const auto& arr = field["array"];

			if(arr.contains("size")) {
				decl_type = std::format("std::array<{}, {}>", elem, arr["size"].as<std::int64_t>());
				suffix = "{}";
				includes.emplace("array");
			} else {
				decl_type = std::format("std::vector<{}>", elem);
				includes.emplace("vector");
			}
		} else {
			decl_type = elem;
		}

		nlohmann::json entry;
		entry["type"] = decl_type;
		entry["name"] = field["name"].as<std::string>();
		entry["suffix"] = suffix;
		out.push_back(std::move(entry));

		if(is_primitive(type)) {
			includes.emplace("cstdint");
			continue;
		}

		const auto* def = reg.find(type);
		const auto kind = kind_of(*def);

		if(kind == "string") {
			includes.emplace("string");
		} else if(kind == "external") {
			includes.emplace((*def)["include"].as<std::string>());
		} else {
			includes.emplace(std::format("generated/types/{}.h", type));
		}
	}
}

std::vector<GeneratedFile> generate_type_headers(const TypeRegistry& reg, const std::filesystem::path& templates_dir) {
	inja::Environment env;
	const auto enum_tpl = env.parse_template((templates_dir / "Enum.h_"));
	const auto struct_tpl = env.parse_template((templates_dir / "Struct.h_"));
	const auto year = current_year();

	std::vector<GeneratedFile> out;

	for(const auto& [name, def] : reg.defs) {
		const auto kind = kind_of(def);

		// string types are stream adaptors and external types are defined elsewhere,
		// so there's nothing to be done for either of them
		if(kind == "string" || kind == "external") {
			continue;
		}

		nlohmann::json data;
		data["name"] = name;
		data["namespace"] = header_namespace(def);
		data["year"] = 	year;

		if(kind == "enum" || kind == "flags") {
			data["underlying"] = cpp_type_for_primitive(def["underlying"].as<std::string>());

			auto values = nlohmann::json::array();

			for(const auto& pair : def["values"].object_range()) {
				nlohmann::json v;
				v["name"] = pair.key();
				v["value"] = pair.value().as<std::int64_t>();
				values.emplace_back(std::move(v));
			}
		
			if(kind == "enum") { // ensure enumerators are sorted numerically (0 ... n)
				std::sort(values.begin(), values.end(), [](auto& lhs, auto& rhs) {
					return rhs["value"] > lhs["value"];
				});

				// convert to hex, should probably add an option to the enum defs
				// to be allow for this to be opt-in
				for(auto& v : values) {
					v["value"] = std::format("{:#04x}", v["value"].get<std::int64_t>());
				}
			}

			data["values"] = std::move(values);

			out.emplace_back(
				std::format("types/{}.h", name),
				env.render(enum_tpl, data)
			);

			continue;
		}

		if(kind == "struct") {
			std::set<std::string> includes;
			auto fields = nlohmann::json::array();

			collect_struct_members(def["fields"], reg, fields, includes);

			auto includes_arr = nlohmann::json::array();

			for(const auto& h : includes) {
				includes_arr.push_back(h);
			}

			data["includes"] = std::move(includes_arr);
			data["fields"] = std::move(fields);
			out.emplace_back(
				std::format("types/{}.h", name),
				env.render(struct_tpl, data)
			);
			continue;
		}

		throw std::runtime_error(
			std::format("type '{}' has unsupported kind '{}' for header generation", name, kind)
		);
	}

	return out;
}

} // protogen, ember