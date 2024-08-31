/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Generator.h"
#include "TypeUtils.h"
#include "Types.h"
#include <logger/Logging.h>
#include <filesystem>
#include <regex>
#include <vector>
#include <fstream>
#include <sstream>

namespace ember::dbc {

void generate_disk_struct_recursive(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_disk_struct(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_disk_enum(const types::Enum& def, std::stringstream& definitions, int indent);
void generate_memory_struct_recursive(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_memory_struct(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_memory_enum(const types::Enum& def, std::stringstream& definitions, int indent);

class StructFieldEnum final : public types::TypeVisitor {
	std::vector<std::string> names_;

public:
	void visit(const types::Struct* type, const types::Field* parent) override {
		// we don't care about structs
	}

	void visit(const types::Enum* type) override {
		// we don't care about enums
	}

	void visit(const types::Field* type, const types::Base* parent) override {
		names_.emplace_back(type->name);
	}

	const std::vector<std::string>& names() {
		return names_;
	}
};

std::optional<std::string> locate_type(const types::Struct& base, const std::string& type_name) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	for(const auto& f : base.children) {
		if(f->name == type_name) {
			return base.name;
		}
	}

	if(base.parent == nullptr) {
		return std::nullopt;
	}

	[[clang::musttail]]
	return locate_type(static_cast<types::Struct&>(*base.parent), type_name);
}

std::string parent_alias(const types::Definitions& defs, const std::string& parent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	for(const auto& def : defs) {
		if(def->name == parent) {
			if(!def->alias.empty()) {
				return def->alias;
			} else {
				return pascal_to_underscore(parent);
			}
		}
	}

	return parent; // couldn't find parent, will just assume the validator caught the problem, if any
}

void save_output(const std::string& path, const std::string& name, const std::string& output) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::filesystem::path dir(path);

	if(!(std::filesystem::exists(dir))) {
		if(!std::filesystem::create_directory(dir)) {
			throw std::runtime_error(path + " does not exist and could not be created");
		}
	}

	dir /= name;

	std::ofstream ofs(dir.string());
	ofs << output;

	if(!ofs.is_open() || !ofs.good()) {
		throw std::runtime_error(name + " could not be written");
	}
}

std::stringstream read_template(const std::string& path, const std::string& file) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::ifstream ifs(path + file);
	std::stringstream buffer;
	buffer << ifs.rdbuf();

	if(!ifs.is_open() || !ifs.good()) {
		throw std::runtime_error(path + file + " could not be opened for reading");
	}

	return buffer;
}

void generate_linker(const types::Definitions& defs, const std::string& output, const std::string& path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::regex pattern(R"(([^]+)<%TEMPLATE_LINKING_FUNCTIONS%>([^]+)<%TEMPLATE_LINKING_FUNCTION_CALLS%>([^]+))");
	std::stringstream buffer(read_template(path, "Linker.cpp_"));
	std::stringstream functions, calls;

	for(const auto& def : defs) {
		if(def->type != types::Type::STRUCT) {
			continue;
		}
			
		auto dbc = static_cast<types::Struct*>(def.get());

		// don't produce linking code for structs
		if(!dbc->dbc) {
			continue;
		}

		std::string store_name = def->alias.empty()? pascal_to_underscore(dbc->name) : dbc->alias;
		std::stringstream call, func;
		bool write_func = false;
		bool double_spaced = false;
		bool first_field = true;
		bool pack_loop_format = true;

		call << "\t" << "detail::link_" << store_name << "(storage);" << std::endl;
		
		func << "void link_" << store_name << "(Storage& storage) {" << std::endl;
		func << "\t" << "for(auto& [k, i] : storage." << store_name << ") {" << std::endl;

		for(const auto& f : dbc->fields) {
			std::stringstream curr_field;
			auto components = extract_components(f.underlying_type);
			bool array = components.second.has_value();
			bool write_field = false;
			std::string type;

			if(auto it = type_map.find(components.first); it == type_map.end()) {
				auto t = locate_type(*dbc, components.first);

				if(!t) {
					throw std::runtime_error("Could not locate type: " + components.first + " in DBC: " + dbc->name);
				}
				
				type = *t + ":: " + components.first;
			} else {
				type = it->first;
			}

			if(array) {
				curr_field << (pack_loop_format? "" : "\n") << (double_spaced || first_field? "" : "\n")
					<< "\t\t" << "for(std::size_t j = 0; j < "
					<< "i." << f.name << ".size();"
					<< " ++j) { " << std::endl;
			} else {
				curr_field << (!pack_loop_format? "\n" : "");
			}

			double_spaced = array;

			for(const auto& k : f.keys) {
				if(k.type == "foreign") {
					curr_field << (array? "\t" : "") << "\t\t" << "i." << f.name
						<< (array? "[j]" : "") << " = " << "storage."
						<< parent_alias(defs, k.parent) << "[i." << f.name << "_id"
						<< (array? "[j]" : "") << "];" << std::endl;
					write_func = true;
					write_field = true;
					break;
				}
			}

			if(array) {
				curr_field << "\t\t" << "}" << "\n";
			}

			if(write_field) {
				pack_loop_format = !array;
				first_field = false;
				func << curr_field.str();
			}
		}

		func << "\t" << "}" << std::endl;
		func << "}" << std::endl << std::endl;

		if(write_func) {
			calls << call.str();
			functions << func.str();
		}
	}

	std::string replace_pattern("$1" + functions.str() + "$2" + calls.str() + "$3");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "Linker.cpp", out);
}

void generate_disk_loader(const types::Definitions& defs, const std::string& output, const std::string& path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;
	LOG_INFO_GLOB << "Generating disk loader..." << LOG_ASYNC;

	std::regex pattern(R"(([^]+)<%TEMPLATE_DISK_LOAD_FUNCTIONS%>([^]+)<%TEMPLATE_DISK_LOAD_MAP_INSERTION%>([^]+)<%TEMPLATE_DISK_LOAD_FUNCTION_CALLS%>([^]+))");
	std::stringstream buffer(read_template(path, "DiskLoader.cpp_"));
	std::stringstream functions, insertions, calls;

	for(const auto& def : defs) {
		if(def->type != types::Type::STRUCT) {
			continue;
		}
		
		const auto& dbc = static_cast<types::Struct&>(*def);

		if(!dbc.dbc) {
			continue;
		}

		TypeMetrics metrics;
		walk_dbc_fields(metrics, &dbc, dbc.parent);

		std::string store_name = dbc.alias.empty()? pascal_to_underscore(dbc.name) : dbc.alias;
		bool double_spaced = false;
		std::string primary_key;

		insertions << "\t" << "dbc_map.emplace(\"" << dbc.name << "\", " << "detail::load_" << store_name << ");" << std::endl;

		calls << "\t" << "log_cb_(\"Loading " << dbc.name << " DBC data...\");" << std::endl;
		calls << "\t" << "detail::load_" << store_name << "(storage, dir_path_);" << std::endl;

		functions << "void load_" << store_name << "(Storage& storage, const std::string& dir_path) {"
			<< std::endl;
		functions << "\t" << "bi::file_mapping file(std::string(dir_path + \"" << dbc.name
			<< ".dbc\").c_str(), bi::read_only);" << std::endl;
		functions << "\t" << "bi::mapped_region region(file, bi::read_only);"
			<< std::endl;
		functions << "\t" << "auto dbc = get_offsets<disk::" << dbc.name <<
			">(region.get_address());" << std::endl << std::endl;

		functions << "\tvalidate_dbc(\"" << dbc.name << "\", dbc.header, " << metrics.record_size
		          << ", " << metrics.fields << ", region.get_size()" << ");" << std::endl << std::endl;
							
		functions << "\t" << "for(std::size_t i = 0; i < dbc.header->records; ++i) {" << std::endl;
		functions << "\t\t" << dbc.name << " entry{};" << std::endl;
		
		bool is_primary_foreign = false;

		for(const auto& f : dbc.fields) {
			auto components = extract_components(f.underlying_type);
			bool array = components.second.has_value();
			bool str_offset = (components.first == "string_ref");
			bool type_found = false;
			std::string type;

			if(auto it = type_map.find(components.first); it != type_map.end()) {
				type = it->second.first;
				type_found = true;
			} else {
				type = components.first;
			}

			if(str_offset) { // another hacky fix!
				type = "std::uint32_t";
			}

			if(array) {
				functions << (double_spaced? "" : "\n") << "\t\t"
					<< "for(std::size_t j = 0; j < std::size(dbc.records[i]."
					<< f.name << "); ++j) {" << std::endl;
			}

			double_spaced = array;

			bool id_suffix = false;
			bool foreign = false;
			bool primary = false;

			for(const auto& k : f.keys) {
				if(k.type == "foreign") {
					id_suffix = true;
					foreign = true;
				}

				if(k.type == "primary") {
					primary_key = f.name;
					primary = true;
				}

				if(primary && foreign) {
					is_primary_foreign = true;
				}
			}

			std::stringstream cast;
			StructFieldEnum enumerator;

			if(!type_map.contains(components.first)) { // user-defined type handling
				auto t = locate_type(dbc, type);

				if(!t) {
					throw std::runtime_error("Could not locate type: " + components.first + " in DBC: " + dbc.name);
				}

				auto base = locate_type_base(dbc, type);
				
				if(!base) {
					throw std::runtime_error("Could not locate type: " + components.first + " in DBC: " + dbc.name);
				}

				if(base->type == types::Type::ENUM) {
					cast << "static_cast<" << *t << "::" << type << ">(";
				} else if(base->type == types::Type::STRUCT) {
					walk_dbc_fields(enumerator, static_cast<types::Struct*>(base), base->parent);
				} else {
					throw std::runtime_error("Unhandled type (not a struct or enum) found: " + components.first + " in DBC: " + dbc.name);
				}
			}

			// I don't even.
			auto names = enumerator.names();
			std::stringstream field;
			std::vector<std::string> field_left, field_right;

			if(components.first == "string_ref_loc") {
				functions << "\n" << (array ? "\t" : "") << "\t\t // string_ref_loc block\n";

				for(const auto& locale : string_ref_loc_regions) {
					functions << (array ? "\t" : "") << "\t\t" << "entry." << f.name << (id_suffix ? "_id." : ".") << locale
						<< (array ? "[j]" : "") << " = " << "dbc.strings + dbc.records[i]." << f.name << "." << locale << ";" << std::endl;
				}

				functions << (array ? "\t" : "") << "\t\t" << "entry." << f.name << (id_suffix ? "_id." : ".") << "flags"
						<< (array ? "[j]" : "") << " = " << "dbc.records[i]." << f.name << "." << "flags" << ";" << std::endl;
				functions << "\n";
			} else {
				if(names.empty()) {
					functions << (array ? "\t" : "") << "\t\t" << "entry." << f.name << (id_suffix ? "_id" : "")
						<< (array ? "[j]" : "") << " = " << cast.str();
				} else {
					for(const auto& name : names) {
						field << (array ? "\t" : "") << "\t\t" << "entry." << f.name << (id_suffix ? "_id" : "")
							<< (array ? "[j]" : "") << "." << name << " = " << cast.str();
						field_left.push_back(field.str());
						field.str("");
					}
				}
			}

			if(components.first == "string_ref") {
				functions << "dbc.strings + ";
			}
			
			if(components.first != "string_ref_loc") {
				if(names.empty()) {
					functions << "dbc.records[i]." << f.name << (array ? "[j]" : "") << (cast.str().empty() ? "" : ")") << ";" << std::endl;
				} else {
					for(const auto& name : names) {
						field << "dbc.records[i]." << f.name << (array ? "[j]" : "") << "." << name << (cast.str().empty() ? "" : ")") << ";" << std::endl;
						field_right.push_back(field.str());
						field.str("");
					}
				}
			}

			for(std::size_t i = 0; i < field_left.size(); ++i) {
				functions << field_left[i] << field_right[i];
			}

			if(array) {
				functions << "\t\t" << "}" << std::endl << std::endl;
			}
		}
		std::string prefix;

		if(is_primary_foreign) {
			prefix = "dbc.records[i].";
		} else {
			prefix = "entry.";
		}

		std::string id = primary_key.empty()? "i" : prefix + primary_key;
		functions << "\t\t" << "storage." << store_name << ".emplace_back(" << id << ", entry);" << std::endl;
		functions << "\t" << "}" << std::endl;
		functions << "}" << std::endl << std::endl;
	}

	std::string replace_pattern("$1" + functions.str() + "$2" + insertions.str() + "$3" + calls.str() + "$4");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "DiskLoader.cpp", out);
}

void generate_disk_struct_recursive(const types::Struct& def, std::stringstream& definitions, int indent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	for(const auto& child : def.children) {
		switch(child->type) { // no default for compiler warning
			case types::Type::STRUCT:
				generate_disk_struct(static_cast<types::Struct&>(*child), definitions, indent + 1);
				break;
			case types::Type::ENUM:
				generate_disk_enum(static_cast<types::Enum&>(*child), definitions, indent + 1);
				break;
			default:
				throw std::runtime_error("Unhandled type!");
		}
	}
}

void generate_disk_struct(const types::Struct& def, std::stringstream& definitions, int indent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::string tab("\t", indent);

	definitions << tab << "struct " << def.name << " {" << std::endl;

	generate_disk_struct_recursive(def, definitions, indent);

	for(const auto& f : def.fields) {
		auto components = extract_components(f.underlying_type);
		std::string field = components.first + " " + f.name;

		if(components.second) {
			field += "[" + std::to_string(*components.second) + "]";
		}

		definitions << tab << "\t" << field << ";" << std::endl;
	}

	definitions << tab << "};" << std::endl << std::endl;
}

void generate_disk_enum(const types::Enum& def, std::stringstream& definitions, int indent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;
	std::string tab("\t", indent);
	definitions << tab << "typedef " << def.underlying_type << " " << def.name << ";" << "\n";
}

void generate_disk_defs(const types::Definitions& defs, const std::string& output, const std::string& path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::regex pattern(R"(([^]+)<%TEMPLATE_DBC_DEFINITIONS%>([^]+))");
	std::stringstream buffer(read_template(path, "DiskDefs.h_"));
	std::stringstream definitions;

	for(const auto& def : defs) {
		if(def->type == types::Type::STRUCT) {
			generate_disk_struct(static_cast<types::Struct&>(*def), definitions, 0);
		} else if(def->type == types::Type::ENUM) {
			const auto& enum_def = static_cast<types::Enum&>(*def);
			generate_disk_enum(enum_def, definitions, 0);
		}
	}

	std::string replace_pattern("$1" + definitions.str() + "$2");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "DiskDefs.h", out);
}

bool is_enum(const std::string& type) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;
	return type.find("enum") != std::string::npos;
}

void generate_memory_enum(const types::Enum& def, std::stringstream& definitions, int indent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::string tab("\t", indent);

	definitions << tab << "enum class " << def.name << " : " << type_map.at(def.underlying_type).first << " {";

	if(!def.comment.empty()) {
		definitions << " // " << def.comment;
	}

	definitions << std::endl;

	for(auto i = def.options.begin(); i != def.options.end(); ++i) {
		std::string name = i->first;
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);

		definitions << tab << "\t" << name << " = " << i->second <<
			(i != def.options.end() - 1? ", " : "") << std::endl;
	}

	definitions << tab << "};" << std::endl << std::endl;
}

void generate_memory_struct_recursive(const types::Struct& def, std::stringstream& definitions, int indent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	for(const auto& child : def.children) {
		switch(child->type) { // no default for compiler warning
			case types::Type::STRUCT:
				generate_memory_struct(static_cast<types::Struct&>(*child), definitions, indent + 1);
				break;
			case types::Type::ENUM:
				generate_memory_enum(static_cast<types::Enum&>(*child), definitions, indent + 1);
				break;
			default:
				throw std::runtime_error("Unhandled type!");
		}
	}
}

void generate_memory_struct(const types::Struct& def, std::stringstream& definitions, int indent) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::string tab("\t", indent);

	definitions << tab << "struct " << def.name << " {";
	
	if(!def.comment.empty()) {
		definitions << " // " << def.comment;
	}

	definitions << std::endl;

	generate_memory_struct_recursive(def, definitions, indent);

	for(const auto& f : def.fields) {
		auto components = extract_components(f.underlying_type);
		bool array = components.second.has_value();
		bool key = false;

		for(const auto& k : f.keys) {
			if(k.type == "foreign") {
				if(array) {
					definitions << tab << "\t" << "std::array<const " << k.parent << "*, "
					            << std::to_string(*components.second) << "> "
					            << f.name << ";" << std::endl;
				} else {
					definitions << tab << "\t" << "const " << k.parent << "* " << f.name << ";" << std::endl;
				}
				key = true;
				break;
			}
		}
		

		std::string field_type;
		definitions << tab << "\t";

		// if the type isn't in the type map, just assume that it's a user-defined struct/enum
		if(auto it = type_map.find(components.first); it != type_map.end()) {
			field_type = it->second.first;
		} else {
			field_type = components.first;
		}

		if(array) {
			definitions << "std::array<" << field_type << ", " << std::to_string(*components.second)
		                << "> " << f.name << (key ? "_id" : "") << ";";
		} else {
			definitions << field_type << " " << f.name << (key ? "_id" : "") << ";";
		}
		if(!f.comment.empty()) {
			definitions << " // " << f.comment;
		}

		definitions << std::endl;
	}

	definitions << tab << "};" << std::endl << std::endl;
}

void generate_memory_defs(const types::Definitions& defs, const std::string& output, const std::string& path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;

	std::regex pattern(R"(([^]+)<%TEMPLATE_MEMORY_FORWARD_DECL%>([^]+)<%TEMPLATE_MEMORY_DEFINITIONS%>([^]+))");

	std::stringstream buffer(read_template(path, "MemoryDefs.h_"));
	std::stringstream forward_decls, definitions;

	for(const auto& def : defs) {
		if(def->type == types::Type::STRUCT) {
			forward_decls << "struct " << def->name << ";" << std::endl;
			generate_memory_struct(static_cast<types::Struct&>(*def), definitions, 0);
		} else if(def->type == types::Type::ENUM) {
			const auto& enum_def = static_cast<types::Enum&>(*def);
			forward_decls << "enum class " << enum_def.name << " : " << enum_def.underlying_type << ";" << std::endl;
			generate_memory_enum(enum_def, definitions, 0);
		}
	}

	std::string replace_pattern("$1" + forward_decls.str() + "$2" + definitions.str() + "$3");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "MemoryDefs.h", out);
}

void generate_storage(const types::Definitions& defs, const std::string& output, const std::string& path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;
	std::regex pattern(R"(([^]+)<%TEMPLATE_DBC_MAPS%>)");

	std::stringstream buffer(read_template(path, "Storage.h_"));
	std::stringstream declarations;

	for(const auto& def : defs) {
		if(def->type != types::Type::STRUCT) {
			continue;
		}

		const auto dbc = static_cast<types::Struct*>(def.get());

		// ensure this is actually a DBC definition rather than a user-defined struct
		if(!dbc->dbc) {
			continue;
		}

		std::string store_name = dbc->alias.empty()? pascal_to_underscore(dbc->name) : dbc->alias;
		declarations << "\tDBCMap<" << dbc->name << "> " << store_name << ";\n";
	}

	std::string replace_pattern("$1" + declarations.str() + "$2");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "Storage.h", out);
}

void generate_common(const types::Definitions& defs, const std::string& output,
                     const std::string& template_path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;
	LOG_INFO_GLOB << "Generating common files..." << LOG_ASYNC;
	generate_storage(defs, output, template_path);
	generate_memory_defs(defs, output, template_path);
	generate_disk_defs(defs, output, template_path);
	generate_linker(defs, output, template_path);
}

void generate_disk_source(const types::Definitions& defs, const std::string& output,
                          const std::string& template_path) {
	LOG_TRACE_GLOB << log_func << LOG_ASYNC;
	generate_disk_loader(defs, output, template_path);
}

} // dbc, ember