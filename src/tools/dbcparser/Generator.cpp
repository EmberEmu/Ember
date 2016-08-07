/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Generator.h"
#include "TypeUtils.h"
#include "Types.h"
#include <regex>
#include <vector>
#include <fstream>
#include <sstream>

#include <iostream>

namespace ember { namespace dbc {

void generate_disk_struct_recursive(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_disk_struct(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_disk_enum(const types::Enum& def, std::stringstream& definitions, int indent);
void generate_memory_struct_recursive(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_memory_struct(const types::Struct& def, std::stringstream& definitions, int indent);
void generate_memory_enum(const types::Enum& def, std::stringstream& definitions, int indent);

boost::optional<std::string> locate_type(const types::Struct& base, const std::string& type_name) {
	for(auto& f : base.children) {
		if(f->name == type_name) {
			return base.name;
		}
	}

	if(base.parent == nullptr) {
		return boost::optional<std::string>();
	}

	return locate_type(static_cast<types::Struct&>(*base.parent), type_name);
}

std::string parent_alias(const types::Definitions& defs, const std::string& parent) {
	for(auto& def : defs) {
		if(def->name == parent) {
			if(!def->alias.empty()) {
				return def->alias;
			} else {
				return pascal_to_underscore(parent);
			}
		}
	}

	return parent; //couldn't find parent, will just assume the validator caught the problem, if any
}

void save_output(const std::string& path, const std::string& name, const std::string& output) {
	std::ofstream ofs(path + "\\" + name);
	ofs << output;

	if(!ofs.is_open() || !ofs.good()) {
		throw std::runtime_error(name + " could not be written");
	}
}

std::stringstream read_template(const std::string& template_file) {
	std::ifstream ifs("templates/" + template_file);
	std::stringstream buffer;
	buffer << ifs.rdbuf();

	if(!ifs.is_open() || !ifs.good()) {
		throw std::runtime_error(template_file + " could not be opened for reading");
	}

	return buffer;
}

void generate_linker(const types::Definitions& defs, const std::string& output) {
	std::stringstream buffer(read_template("Linker.cpp_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_LINKING_FUNCTIONS%>([^]+)<%TEMPLATE_LINKING_FUNCTION_CALLS%>([^]+))");
	std::stringstream functions, calls;

	for(auto& def : defs) {
		if(def->type != types::STRUCT) {
			continue;
		}
			
		auto dbc = static_cast<types::Struct*>(def.get());
		std::string store_name = def->alias.empty()? pascal_to_underscore(dbc->name) : dbc->alias;
		std::stringstream call, func;
		bool write_func = false;
		bool double_spaced = false;
		bool first_field = true;
		bool pack_loop_format = true;

		call << "\t" << "detail::link_" << store_name << "(storage);" << std::endl;
		
		func << "void link_" << store_name << "(Storage& storage) {" << std::endl;
		func << "\t" << "for(auto& i : storage." << store_name << ") {" << std::endl;

		for(auto& f : dbc->fields) {
			std::stringstream curr_field;
			auto components = extract_components(f.underlying_type);
			bool array = components.second.is_initialized();
			bool write_field = false;

			if(array) {
				curr_field << (pack_loop_format? "" : "\n") << (double_spaced || first_field? "" : "\n")
					<< "\t\t" << "for(std::size_t j = 0; j < "
					<< "sizeof(i." << f.name << ") / sizeof(" << type_map[components.first].first << ");"
					<< " ++j) { " << std::endl;
			} else {
				curr_field << (!pack_loop_format? "\n" : "");
			}

			double_spaced = array;

			for(auto& k : f.keys) {
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

void generate_disk_loader(const types::Definitions& defs, const std::string& output) {
	std::stringstream buffer(read_template("DiskLoader.cpp_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_DISK_LOAD_FUNCTIONS%>([^]+)<%TEMPLATE_DISK_LOAD_MAP_INSERTION%>([^]+)<%TEMPLATE_DISK_LOAD_FUNCTION_CALLS%>([^]+))");
	std::stringstream functions, insertions, calls;

	for(auto& def : defs) {
		if(def->type != types::STRUCT) {
			continue;
		}
		
		auto& dbc = static_cast<types::Struct&>(*def);

		if(!dbc.dbc) {
			continue;
		}

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
							
		functions << "\t" << "for(std::uint32_t i = 0; i < dbc.header->records; ++i) {" << std::endl;
		functions << "\t\t" << dbc.name << " entry{};" << std::endl;
		
		bool is_primary_foreign = false;
		
		for(auto& f : dbc.fields) {
			auto components = extract_components(f.underlying_type);
			bool array = components.second.is_initialized();
			bool str_offset = (components.first == "string_ref");
			bool type_found = false;
			std::string type;

			if(type_map.find(components.first) != type_map.end()) {
				type = type_map[components.first].first;
				type_found = true;
			} else {
				type = components.first;
			}

			if(array) {
				functions << (double_spaced? "" : "\n") << "\t\t"
					<< "for(std::size_t j = 0; j < sizeof(dbc.records[i]."
					<< f.name << ") / sizeof(" << (type_found? "" : dbc.name + "::") << type << "); ++j) {" << std::endl;
			}

			double_spaced = array;

			std::stringstream cast;
			bool id_suffix = false;
			bool foreign = false;
			bool primary = false;

			for(auto& k : f.keys) {
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

			if(cast.str().empty() && type_map.find(components.first) == type_map.end()) {
				auto t = locate_type(dbc, type);
				cast << "static_cast<" << *t << "::" << type << ">(";
			}

			// hardcoded, bad, like the rest of this file
			std::vector<std::string> locales = { 
				"enGB", "koKR", "frFR", "deDE", "enCN", "enTW", "esES", "esMX"
			};

			if(components.first == "string_ref_loc") {
				functions << "\n" << (array ? "\t" : "") << "\t\t // string_ref_loc block\n";

				for(auto& locale : locales) {
					functions << (array ? "\t" : "") << "\t\t" << "entry." << f.name << (id_suffix ? "_id." : ".") << locale
						<< (array ? "[j]" : "") << " = " << "dbc.strings + dbc.records[i]." << f.name << "." << locale << ";" << std::endl;
				}

				functions << "\n";
			} else {
				functions << (array ? "\t" : "") << "\t\t" << "entry." << f.name << (id_suffix ? "_id" : "")
					<< (array ? "[j]" : "") << " = " << cast.str();
			}


			if(components.first == "string_ref") {
				functions << "dbc.strings + ";
			}
			
			if(components.first != "string_ref_loc") {
				functions << "dbc.records[i]." << f.name << (array ? "[j]" : "") << (cast.str().empty() ? "" : ")") << ";" << std::endl;
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
	for(auto& child : def.children) {
		switch(child->type) { // no default for compiler warning
			case types::STRUCT:
				generate_disk_struct(static_cast<types::Struct&>(*child), definitions, indent + 1);
				break;
			case types::ENUM:
				generate_disk_enum(static_cast<types::Enum&>(*child), definitions, indent + 1);
				break;
		}
	}
}

void generate_disk_struct(const types::Struct& def, std::stringstream& definitions, int indent) {
	// only load DBCs, not shared structs
	if(!def.dbc) {
		return;
	}

	std::string tab("\t", indent);

	definitions << tab << "struct " << def.name << " {" << std::endl;

	generate_disk_struct_recursive(def, definitions, indent);

	for(auto& f : def.fields) {
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
	std::string tab("\t", indent);
	definitions << tab << "typedef " << def.underlying_type << " " << def.name << ";" << "\n";
}

void generate_disk_defs(const types::Definitions& defs, const std::string& output) {
	std::stringstream buffer(read_template("DiskDefs.h_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_DBC_DEFINITIONS%>([^]+))");
	std::stringstream definitions;

	for(auto& def : defs) {
		if(def->type == types::STRUCT) {
			generate_disk_struct(static_cast<types::Struct&>(*def), definitions, 0);
		} else if(def->type == types::ENUM) {
			auto& enum_def = static_cast<types::Enum&>(*def);
			generate_disk_enum(enum_def, definitions, 0);
		} else {
			// todo
		}
	}

	std::string replace_pattern("$1" + definitions.str() + "$2");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "DiskDefs.h", out);
}

bool is_enum(const std::string& type) {
	return type.find("enum") != std::string::npos;
}

void generate_memory_enum(const types::Enum& def, std::stringstream& definitions, int indent) {
	std::string tab("\t", indent);

	definitions << tab << "enum class " << def.name << " : " << type_map[def.underlying_type].first << " {";

	if(!def.comment.empty()) { // todo, fix
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
	for(auto& child : def.children) {
		switch(child->type) { // no default for compiler warning
			case types::STRUCT:
				generate_memory_struct(static_cast<types::Struct&>(*child), definitions, indent + 1);
				break;
			case types::ENUM:
				generate_memory_enum(static_cast<types::Enum&>(*child), definitions, indent + 1);
				break;
		}
	}
}

void generate_memory_struct(const types::Struct& def, std::stringstream& definitions, int indent) {
	std::string tab("\t", indent);

	definitions << tab << "struct " << def.name << " {";
	
	if(!def.comment.empty()) { // todo, fix
		definitions << " // " << def.comment;
	}

	definitions << std::endl;

	generate_memory_struct_recursive(def, definitions, indent);

	for(auto& f : def.fields) {
		auto components = extract_components(f.underlying_type);
		bool array = components.second.is_initialized();
		bool key = false;

		for(auto& k : f.keys) {
			if(k.type == "foreign") {
				definitions << tab << "\t" + k.parent << "* " << f.name
					<< (array ? "[" + std::to_string(*components.second) + "]" : "")
					<< ";" << std::endl;
				key = true;
				break;
			}
		}
		
		definitions << tab << "\t";

		// if the type isn't in the type map, just assume that it's a user-defined struct/enum
		if(type_map.find(components.first) != type_map.end()) {
			definitions << type_map[components.first].first;
		} else {
			definitions << components.first;
		}

		definitions << " " << f.name << (key ? "_id" : "")
			<< (array ? "[" + std::to_string(*components.second) + "]" : "") << ";";

		if(!f.comment.empty()) { // todo, fix
			definitions << " // " << f.comment;
		}

		definitions << std::endl;
	}

	definitions << tab << "};" << std::endl << std::endl;
}

void generate_memory_defs(const types::Definitions& defs, const std::string& output) {
	std::stringstream buffer(read_template("MemoryDefs.h_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_MEMORY_FORWARD_DECL%>([^]+)<%TEMPLATE_MEMORY_DEFINITIONS%>([^]+))");
	std::stringstream forward_decls, definitions;

	for(auto& def : defs) {
		if(def->type == types::STRUCT) {
			forward_decls << "struct " << def->name << ";" << std::endl;
			generate_memory_struct(static_cast<types::Struct&>(*def), definitions, 0);
		} else if(def->type == types::ENUM) {
			auto& enum_def = static_cast<types::Enum&>(*def);
			forward_decls << "enum class " << enum_def.name << " : " << enum_def.underlying_type << ";" << std::endl;
			generate_memory_enum(enum_def, definitions, 0);
		} else {
			// todo
		}
	}

	std::string replace_pattern("$1" + forward_decls.str() + "$2" + definitions.str() + "$3");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "MemoryDefs.h", out);
}

void generate_storage(const types::Definitions& defs, const std::string& output) {
	std::stringstream buffer(read_template("Storage.h_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_DBC_MAPS%>([^]+)<%TEMPLATE_MOVES%>([^]+))");
	std::stringstream declarations, moves;

	for(auto& def : defs) {
		if(def->type != types::STRUCT) {
			continue;
		}

		auto dbc = static_cast<types::Struct*>(def.get());

		// ensure this is actually a DBC definition rather than a user-defined struct
		if(!dbc->dbc) {
			continue;
		}

		std::string store_name = dbc->alias.empty()? pascal_to_underscore(dbc->name) : dbc->alias;
		declarations << "\tDBCMap<" << dbc->name << "> " << store_name << ";\n";
		moves << "\t\t" << store_name << " = std::move(" << store_name << ");\n";
	}

	std::string replace_pattern("$1" + declarations.str() + "$2" + moves.str() + "$3");
	std::string out = std::regex_replace(buffer.str(), pattern, replace_pattern);
	save_output(output, "Storage.h", out);
}

void generate_common(const types::Definitions& defs, const std::string& output) {
	generate_storage(defs, output);
	generate_memory_defs(defs, output);
	generate_disk_defs(defs, output);
	generate_disk_loader(defs, output);
	generate_linker(defs, output);
}

void generate_disk_source(const types::Definitions& defs, const std::string& output) {

}

}} // dbc, ember