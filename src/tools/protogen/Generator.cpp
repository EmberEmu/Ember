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

enum class Dir {
	Read,
	Write,
	Both
};

struct CountFieldLocal {
	std::string local_var;
	std::string wire_type;
};

struct WalkState {
	std::vector<std::string> members;
	std::set<std::string> includes;
	std::vector<std::string> read_ops;
	std::vector<std::string> write_ops;
	int read_tab = 2;
	int write_tab = 2;
	int temp_counter = 0;
	std::set<std::string> count_field_names;
	std::unordered_map<std::string, CountFieldLocal> count_locals;
};

void walk(const jsoncons::json& fields, const TypeRegistry& reg, WalkState& state,
		  std::vector<ScopeEntry>& scope, const std::string& prefix);

void collect_count_field_names(const jsoncons::json& fields, std::set<std::string>& out) {
	for(const auto& f : fields.array_range()) {
		const auto type = f["type"].as<std::string>();

		if(type == "group") {
			collect_count_field_names(f["fields"], out);
			continue;
		}

		if(type == "if_chain") {
			for(const auto& branch : f["branches"].array_range()) {
				collect_count_field_names(branch["fields"], out);
			}

			if(f.contains("else")) {
				collect_count_field_names(f["else"], out);
			}

			continue;
		}

		if(f.contains("array") && f["array"].contains("count_field")) {
			out.insert(f["array"]["count_field"].as<std::string>());
		}
	}
}

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

std::string effective_field_type(const jsoncons::json& field, const TypeRegistry& reg) {
	const auto type = field["type"].as<std::string>();

	if(field.contains("as")) {
		return type;
	}

	const auto def = reg.find(type);

	if(def && kind_of(*def) == "external" && def->contains("underlying")) {
		return (*def)["underlying"].as<std::string>();
	}

	return type;
}

void add_type_include(const std::string& type, const TypeRegistry& reg, std::set<std::string>& out) {
	if(is_primitive(type)) {
		return;
	}

	const auto def = reg.find(type);

	if(!def) {
		throw std::runtime_error(std::format(
			"unknown type '{}' reached code generation — should have been rejected by validation", type
		));
	}

	const auto kind = kind_of(*def);

	if(is_enum_kind(kind) || kind == "flags" || kind == "struct") {
		out.emplace(std::format("protocol/types/{}.h", type));
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

	const auto def = reg.find(type);

	if(!def) {
		throw std::runtime_error(std::format(
			"unknown type '{}' reached code generation — should have been rejected by validation", type
		));
	}

	if(kind_of(*def) == "string") {
		return "std::string";
	}

	const auto leaf = def->contains("cpp_type_name")? (*def)["cpp_type_name"].as<std::string>() : type;

	if(def->contains("cpp_namespace")) {
		return std::format("{}::{}", (*def)["cpp_namespace"].as<std::string>(), leaf);
	}

	return leaf;
}

std::string string_adaptor_expr(const jsoncons::json& type_def, std::string_view expr, Dir dir) {
	const auto encoding = type_def["encoding"].as<std::string>();

	if(encoding == "null_terminated") {
		return std::format("spark::io::null_terminated({})", expr);
	}

	const auto length_type = type_def["length_type"].as<std::string>();
	const auto length_cpp = cpp_type_for_primitive(length_type);
	std::string_view qualifier = (dir == Dir::Write)? "const " : "";

	if(encoding == "prefixed") {
		return std::format("spark::io::prefixed<{}std::string, {}>({})", qualifier, length_cpp, expr);
	}

	if(encoding == "prefixed_null_terminated") {
		return std::format("spark::io::prefixed_null_terminated<{}std::string, {}>({})", qualifier, length_cpp, expr);
	}

	throw std::runtime_error(
		std::format("unknown string encoding '{}'", encoding)
	);
}

std::string render_cond_rhs(const jsoncons::json& v, std::string_view prefix, std::string_view type) {
	if(v.is_string()) {
		return std::format("{}::{}", type, v.as<std::string>());
	}

	if(v.is_object()) {
		const auto ref_name = v["field"].as<std::string>();
		const auto ref_qualified = std::format("{}{}", prefix, ref_name);
		bool negate = false;

		if(v.contains("negate") && v["negate"].as<bool>()) {
			negate = true;
		}

		return negate? std::format("!{}", ref_qualified) : ref_qualified;
	}

	return std::format("{}", v.as<std::int64_t>());
}

std::string render_condition(const jsoncons::json& cond,
                             const TypeRegistry& reg,
                             std::string_view prefix,
                             std::span<const ScopeEntry> scope) {
	const auto op = cond["op"].as<std::string>();

	if(op == "stream_not_empty") {
		return "!stream.empty()";
	}

	if(op == "and" || op == "or") {
		const std::string_view sep = (op == "and")? " && " : " || ";
		std::string joined;

		for(const auto& sub : cond["conditions"].array_range()) {
			if(!joined.empty()) {
				joined += sep;
			}

			joined += std::format("({})", render_condition(sub, reg, prefix, scope));
		}

		return joined;
	}

	if(op == "not") {
		return std::format("!({})", render_condition(cond["condition"], reg, prefix, scope));
	}

	const auto field_name = cond["field"].as<std::string>();
	const auto qualified = std::format("{}{}", prefix, field_name);
	const auto entry = lookup(scope, field_name);

	if(!entry) {
		throw std::runtime_error(std::format("condition references unknown field '{}'", field_name));
	}

	if(op == "empty") {
		return std::format("{}.empty()", qualified);
	}

	const auto& type = entry->type;
	const auto& value = cond["value"];
	const auto enum_qualified = cpp_type_name(type, reg);

	if(op == "eq") {
		return std::format("{} == {}", qualified, render_cond_rhs(value, prefix, enum_qualified));
	}

	if(op == "neq") {
		return std::format("{} != {}", qualified, render_cond_rhs(value, prefix, enum_qualified));
	}

	if(op == "in") {
		std::string joined;

		for(const auto& v : value.array_range()) {
			if(!joined.empty()) {
				joined += " || ";
			}

			joined += std::format("{} == {}", qualified, render_cond_rhs(v, prefix, enum_qualified));
		}

		return joined;
	}

	if(op == "has_flag") {
		return std::format("{} & {}", qualified, render_cond_rhs(value, prefix, enum_qualified));
	}

	throw std::runtime_error(std::format("unknown condition op '{}'", op));
}

std::string cpp_element_type(const std::string& type, const TypeRegistry& reg) {
	return cpp_type_name(type, reg);
}


void emit_scalar_stream_op(const std::string& name, const std::string& type,
                           const std::string& as_type,
                           const TypeRegistry& reg, WalkState& state,
                           std::string_view prefix, Dir dir = Dir::Both) {
	const auto qualified = std::format("{}{}", prefix, name);
	const bool do_read  = dir != Dir::Write;
	const bool do_write = dir != Dir::Read;

	if(!as_type.empty()) {
		const auto wire_cpp = cpp_type_for_primitive(type);
		const auto semantic_cpp = cpp_type_name(as_type, reg);

		if(do_read) {
			const auto raw_name = std::format("raw_{}", state.temp_counter++);
			state.read_ops.emplace_back(std::format(
				"{0}{1} {2};", indent(state.read_tab), wire_cpp, raw_name
			));
			state.read_ops.emplace_back(std::format(
				"{0}stream >> {1};", indent(state.read_tab), raw_name
			));
			state.read_ops.emplace_back(std::format(
				"{0}{1} = static_cast<{2}>({3});",
				indent(state.read_tab), qualified, semantic_cpp, raw_name
			));
		}

		if(do_write) {
			state.write_ops.emplace_back(std::format(
				"{}stream << static_cast<{}>({});",
				indent(state.write_tab), wire_cpp, qualified
			));
		}

		add_type_include(as_type, reg, state.includes);
		return;
	}

	if(is_primitive(type)) {
		if(do_read) {
			state.read_ops.emplace_back(
				std::format("{}stream >> {};", indent(state.read_tab), qualified)
			);
		}
		if(do_write) {
			state.write_ops.emplace_back(
				std::format("{}stream << {};", indent(state.write_tab), qualified)
			);
		}
		return;
	}

	const auto def = reg.find(type);

	if(!def) {
		throw std::runtime_error(
			std::format("unknown type '{}' for field '{}'", type, name)
		);
	}

	const auto kind = kind_of(*def);

	if(is_enum_kind(kind) || kind == "flags") {
		if(do_read) {
			state.read_ops.emplace_back(
				std::format("{}stream >> {};", indent(state.read_tab), qualified)
			);
		}
		if(do_write) {
			state.write_ops.emplace_back(
				std::format("{}stream << {};", indent(state.write_tab), qualified)
			);
		}
		return;
	}

	// user-defined externals are expected to provide >> & << operator overloads (e.g. PackedGuid)
	if(kind == "external") {
		if(do_read) {
			state.read_ops.emplace_back(
				std::format("{}stream >> {};", indent(state.read_tab), qualified)
			);
		}

		if(do_write) {
			state.write_ops.emplace_back(
				std::format("{}stream << {};", indent(state.write_tab), qualified)
			);
		}

		return;
	}

	if(kind == "string") {
		if(do_read) {
			const auto adaptor = string_adaptor_expr(*def, qualified, Dir::Read);
			state.read_ops.emplace_back(std::format("{}stream >> {};", indent(state.read_tab), adaptor));
		}

		if(do_write) {
			const auto adaptor = string_adaptor_expr(*def, qualified, Dir::Write);
			state.write_ops.emplace_back(std::format("{}stream << {};", indent(state.write_tab), adaptor));
		}

		state.includes.insert("spark/buffers/StringAdaptors.h");
		return;
	}

	if(kind == "struct") {
		const auto read_before  = state.read_ops.size();
		const auto write_before = state.write_ops.size();
		auto saved_names = std::move(state.count_field_names);
		auto saved_locals = std::move(state.count_locals);

		state.count_field_names.clear();
		state.count_locals.clear();
		collect_count_field_names((*def)["fields"], state.count_field_names);

		std::vector<ScopeEntry> nested_scope;
		walk((*def)["fields"], reg, state, nested_scope, qualified + ".");

		state.count_field_names = std::move(saved_names);
		state.count_locals = std::move(saved_locals);

		if(!do_read) {
			state.read_ops.resize(read_before);
		}

		if(!do_write) {
			state.write_ops.resize(write_before);
		}

		return;
	}

	throw std::runtime_error(
		std::format("unsupported kind '{}' for field '{}'", kind, name)
	);
}

bool has_whole_array_stream_op(const std::string& type, const std::string& as_type, const TypeRegistry& reg) {
	if(!as_type.empty()) {
		return false;
	}

	if(is_primitive(type)) {
		return true;
	}

	const auto def = reg.find(type);

	if(!def) {
		return false;
	}

	const auto kind = kind_of(*def);
	return is_enum_kind(kind) || kind == "flags" || kind == "external";
}

void emit_stream_op(const jsoncons::json& field, const std::string& type,
                    const TypeRegistry& reg, WalkState& state, const std::string& prefix) {
	const auto name = field["name"].as<std::string>();
	const auto as_type = field.contains("as")? field["as"].as<std::string>() : std::string();

	if(!field.contains("array")) {
		emit_scalar_stream_op(name, type, as_type, reg, state, prefix);
		return;
	}

	const auto& arr = field["array"];
	const auto qualified = prefix + name;

	if(arr.contains("size")) {
		if(has_whole_array_stream_op(type, as_type, reg)) {
			state.read_ops.emplace_back(std::format("{}stream >> {};", indent(state.read_tab), qualified));
			state.write_ops.emplace_back(std::format("{}stream << {};", indent(state.write_tab), qualified));
			state.includes.insert("array");
			return;
		}

		state.includes.insert("array");

		state.read_ops.emplace_back(
			std::format("{}for(auto& e : {}) {{", indent(state.read_tab), qualified)
		);

		state.write_ops.emplace_back(
			std::format("{}for(const auto& e : {}) {{", indent(state.write_tab), qualified)
		);

		++state.read_tab;
		++state.write_tab;
		emit_scalar_stream_op("e", type, as_type, reg, state, std::string());
		--state.read_tab;
		--state.write_tab;

		state.read_ops.emplace_back(std::format("{}}}", indent(state.read_tab)));
		state.write_ops.emplace_back(std::format("{}}}", indent(state.write_tab)));
		return;
	}

	// read until the stream is empty
	if(arr.contains("until_end")) {
		state.includes.emplace("vector");

		state.read_ops.emplace_back(
			std::format("{}while(!stream.empty()) {{", indent(state.read_tab))
		);
		++state.read_tab;
		state.read_ops.emplace_back(
			std::format("{}{}.emplace_back();", indent(state.read_tab), qualified)
		);
		emit_scalar_stream_op(
			std::format("{}.back()", qualified), type, as_type,
			reg, state, std::string(), Dir::Read
		);
		state.read_ops.emplace_back(
			std::format("{}if(!stream) return StreamResult::failed;", indent(state.read_tab))
		);
		--state.read_tab;
		state.read_ops.emplace_back(std::format("{}}}", indent(state.read_tab)));

		state.write_ops.emplace_back(
			std::format("{}for(const auto& e : {}) {{", indent(state.write_tab), qualified)
		);
		++state.write_tab;
		emit_scalar_stream_op("e", type, as_type, reg, state, std::string(), Dir::Write);
		--state.write_tab;
		state.write_ops.emplace_back(std::format("{}}}", indent(state.write_tab)));
		return;
	}

	const auto count_field = arr["count_field"].as<std::string>();
	const auto& count_info = state.count_locals.at(count_field);
	const auto count_cpp = cpp_type_for_primitive(count_info.wire_type);
	state.includes.emplace("vector");

	state.write_ops.emplace_back(std::format(
		"{}stream << static_cast<{}>({}.size());",
		indent(state.write_tab), count_cpp, qualified
	));

	state.read_ops.emplace_back(
		std::format("{}{}.resize({});", indent(state.read_tab), qualified, count_info.local_var)
	);

	state.read_ops.emplace_back(
		std::format("{}for(auto& e : {}) {{", indent(state.read_tab), qualified)
	);

	state.write_ops.emplace_back(
		std::format("{}for(const auto& e : {}) {{", indent(state.write_tab), qualified)
	);

	++state.read_tab;
	++state.write_tab;
	emit_scalar_stream_op("e", type, as_type, reg, state, std::string());
	--state.read_tab;
	--state.write_tab;

	state.read_ops.emplace_back(std::format("{}}}", indent(state.read_tab)));
	state.write_ops.emplace_back(std::format("{}}}", indent(state.write_tab)));
}

std::string default_initializer(const jsoncons::json& field, const TypeRegistry& reg) {
	if(!field.contains("default")) {
		return {};
	}

	const auto& v = field["default"];
	const auto type = field.contains("as")
		? field["as"].as<std::string>() : effective_field_type(field, reg);

	if(is_primitive(type)) {
		const auto info = type_map.at(type);

		if(info.second == TypeInfo::integral) {
			return std::format("{{{}}}", v.as<std::int64_t>());
		}

		if(info.second == TypeInfo::boolean) {
			return v.as<bool>()? "{true}" : "{false}";
		}

		if(type == "float") {
			return std::format("{{static_cast<float>({})}}", v.as<double>());
		}

		return std::format("{{{}}}", v.as<double>());
	}

	const auto def = reg.find(type);

	if(!def) {
		throw std::runtime_error(std::format(
			"default for field '{}' references unknown type '{}'",
			field["name"].as<std::string>(), type
		));
	}

	const auto kind = kind_of(*def);

	if(is_enum_kind(kind) || kind == "flags") {
		return std::format("{{{}::{}}}", cpp_type_name(type, reg), v.as<std::string>());
	}

	if(kind == "string") {
		std::string escaped;
		for(const char c : v.as<std::string>()) {
			if(c == '\\' || c == '"') {
				escaped.push_back('\\');
			}
			escaped.push_back(c);
		}
		return std::format("{{\"{}\"}}", escaped);
	}

	throw std::runtime_error(std::format(
		"default not supported for field '{}' of kind '{}'",
		field["name"].as<std::string>(), kind
	));
}

std::string member_decl(const jsoncons::json& field, const TypeRegistry& reg) {
	const auto name = field["name"].as<std::string>();
	const auto declared_type = field.contains("as")? field["as"].as<std::string>() : effective_field_type(field, reg);
	const auto elem = cpp_element_type(declared_type, reg);

	if(field.contains("array")) {
		const auto& arr = field["array"];

		if(arr.contains("size")) {
			const auto sz = arr["size"].as<std::int64_t>();
			return std::format("\tstd::array<{}, {}> {}{{}};", elem, sz, name);
		}

		// count_field and until_end both produce a vector
		return std::format("\tstd::vector<{}> {};", elem, name);
	}

	return std::format("\t{} {}{};", elem, name, default_initializer(field, reg));
}

void emit_branch_body(const jsoncons::json& branch_fields, const TypeRegistry& reg,
                      WalkState& state, std::vector<ScopeEntry>& scope,
                      const std::string& prefix) {
	const auto outer_size = scope.size();
	walk(branch_fields, reg, state, scope, prefix);
	scope.resize(outer_size);
}

void walk(const jsoncons::json& fields, const TypeRegistry& reg, WalkState& state,
          std::vector<ScopeEntry>& scope, const std::string& prefix) {
	const bool emit_members = prefix.empty();
	bool first = true;
	bool prev_is_block = false;

	for(const auto& field : fields.array_range()) {
		const auto type = field["type"].as<std::string>();
		const bool current_is_group = (type == "group");
		const bool current_is_if_chain = (type == "if_chain");
		const bool current_is_block = current_is_group || current_is_if_chain;
		const bool need_blank = !first && (prev_is_block || current_is_block);

		if(need_blank) {
			state.read_ops.emplace_back("");
			state.write_ops.emplace_back("");
		}

		if(current_is_group) {
			const auto& when = field["when"];
			const auto op = when["op"].as<std::string>();

			// stream_not_empty applies to reading, not writing
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

			emit_branch_body(field["fields"], reg, state, scope, prefix);

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
		} else if(current_is_if_chain) {
			const auto& branches = field["branches"];
			std::size_t idx = 0;
			const std::size_t branch_count = branches.size();

			for(const auto& branch : branches.array_range()) {
				const auto cond = render_condition(branch["when"], reg, prefix, scope);
				const std::string_view prefix_word = (idx == 0) ? "if" : "else if";

				state.read_ops .emplace_back(std::format(
					"{}{}({}) {{", indent(state.read_tab), prefix_word, cond
				));
				state.write_ops.emplace_back(std::format(
					"{}{}({}) {{", indent(state.write_tab), prefix_word, cond
				));

				++state.read_tab;
				++state.write_tab;
				emit_branch_body(branch["fields"], reg, state, scope, prefix);
				--state.read_tab;
				--state.write_tab;

				const bool last_in_chain = (idx + 1 == branch_count) && !field.contains("else");
				state.read_ops .emplace_back(std::format(
					"{}{}", indent(state.read_tab), last_in_chain ? "}" : "}"
				));
				state.write_ops.emplace_back(std::format(
					"{}{}", indent(state.write_tab), last_in_chain ? "}" : "}"
				));
				++idx;
			}

			if(field.contains("else")) {
				state.read_ops .emplace_back(std::format("{}else {{", indent(state.read_tab)));
				state.write_ops.emplace_back(std::format("{}else {{", indent(state.write_tab)));
				++state.read_tab;
				++state.write_tab;
				emit_branch_body(field["else"], reg, state, scope, prefix);
				--state.read_tab;
				--state.write_tab;
				state.read_ops .emplace_back(std::format("{}}}", indent(state.read_tab)));
				state.write_ops.emplace_back(std::format("{}}}", indent(state.write_tab)));
			}
		} else {
			const auto effective = effective_field_type(field, reg);
			const auto semantic_type = field.contains("as")? field["as"].as<std::string>() : effective;
			const auto field_name = field["name"].as<std::string>();
			scope.emplace_back(field_name, semantic_type);

			if(!field.contains("array") && state.count_field_names.contains(field_name)) {
				const auto wire_type = field["type"].as<std::string>();
				const auto cpp = cpp_type_for_primitive(wire_type);
				const auto local = std::format("{}_{}", field_name, state.temp_counter++);
				state.count_locals[field_name] = { local, wire_type };

				state.read_ops.emplace_back(
					std::format("{}{} {};", indent(state.read_tab), cpp, local)
				);

				state.read_ops.emplace_back(
					std::format("{}stream >> {};", indent(state.read_tab), local)
				);
			} else {
				add_type_include(effective, reg, state.includes);

				if(field.contains("as")) {
					add_type_include(field["as"].as<std::string>(), reg, state.includes);
				}

				if(emit_members) {
					state.members.emplace_back(member_decl(field, reg));
					const auto def = reg.find(semantic_type);

					if(def && kind_of(*def) == "string") {
						state.includes.emplace("string");
					}
				}

				emit_stream_op(field, effective, reg, state, prefix);
			}
		}

		first = false;
		prev_is_block = current_is_block;
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
	auto alias = opcode;

	if(message.contains("alias")) {
		alias = message["alias"].as<std::string>();
	}

	WalkState state;
	std::vector<ScopeEntry> scope;

	if(message.contains("fields")) {
		collect_count_field_names(message["fields"], state.count_field_names);
		walk(message["fields"], registry, state, scope, {});
	}

	nlohmann::json data;
	data["year"] = current_year();
	data["name"] = name;
	data["opcode"] = opcode;
	data["direction"] = direction;
	data["packet_template"] = (direction == "server")? "ServerPacket" : "ClientPacket";
	data["opcode_enum"] = (direction == "server")? "ServerOpcode" : "ClientOpcode";
	data["members"] = join_lines(state.members);
	data["read_body"] = join_lines(state.read_ops);
	data["write_body"] = join_lines(state.write_ops);
	data["alias"] = alias;

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
                            nlohmann::json& out, std::set<std::string>& includes,
                            const std::set<std::string>& count_field_names) {
	for(const auto& field : fields.array_range()) {
		const auto type = field["type"].as<std::string>();

		if(type == "group") {
			collect_struct_members(field["fields"], reg, out, includes, count_field_names);
			continue;
		}

		if(type == "if_chain") {
			for(const auto& branch : field["branches"].array_range()) {
				collect_struct_members(branch["fields"], reg, out, includes, count_field_names);
			}
			if(field.contains("else")) {
				collect_struct_members(field["else"], reg, out, includes, count_field_names);
			}
			continue;
		}

		if(!field.contains("array") && count_field_names.contains(field["name"].as<std::string>())) {
			continue;
		}

		const auto effective = effective_field_type(field, reg);
		const auto declared_type = field.contains("as")? field["as"].as<std::string>() : effective;
		const auto elem = cpp_element_type(declared_type, reg);

		std::string decl_type;
		std::string suffix;

		if(field.contains("array")) {
			const auto& arr = field["array"];

			if(arr.contains("size")) {
				decl_type = std::format("std::array<{}, {}>", elem, arr["size"].as<std::int64_t>());
				suffix = "{}";
				includes.emplace("array");
			} else {
				// count_field and until_end both map to vector<T>.
				decl_type = std::format("std::vector<{}>", elem);
				includes.emplace("vector");
			}
		} else {
			decl_type = elem;
			suffix = default_initializer(field, reg);
		}

		nlohmann::json entry;
		entry["type"] = decl_type;
		entry["name"] = field["name"].as<std::string>();
		entry["suffix"] = suffix;
		out.push_back(std::move(entry));

		// includes for both the wire primitive and any 'as' semantic type
		const auto register_include_for = [&](const std::string& ty) {
			if(is_primitive(ty)) {
				includes.emplace("cstdint");
				return;
			}

			const auto def = reg.find(ty);

			if(!def) {
				return;
			}

			const auto kind = kind_of(*def);

			if(kind == "string") {
				includes.emplace("string");
			} else if(kind == "external") {
				includes.emplace((*def)["include"].as<std::string>());
			} else {
				includes.emplace(std::format("protocol/types/{}.h", ty));
			}
		};

		register_include_for(effective);

		if(field.contains("as")) {
			register_include_for(field["as"].as<std::string>());
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

		if(is_enum_kind(kind) || kind == "flags") {
			data["underlying"] = cpp_type_for_primitive(def["underlying"].as<std::string>());
			data["is_class"] = (kind == "enum_class");

			auto values = nlohmann::json::array();

			for(const auto& pair : def["values"].object_range()) {
				nlohmann::json v;
				v["name"] = pair.key();
				v["value"] = pair.value().as<std::int64_t>();
				values.emplace_back(std::move(v));
			}

			if(is_enum_kind(kind)) { // ensure enumerators are sorted numerically (0 .. n)
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
			std::set<std::string> count_field_names;
			auto fields = nlohmann::json::array();

			collect_count_field_names(def["fields"], count_field_names);
			collect_struct_members(def["fields"], reg, fields, includes, count_field_names);

			auto includes_arr = nlohmann::json::array();

			for(const auto& h : includes) {
				includes_arr.push_back(h);
			}

			data["includes"] = std::move(includes_arr);
			data["fields"] = std::move(fields);

			out.emplace_back(
				std::format("types/{}.h", name), env.render(struct_tpl, data)
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