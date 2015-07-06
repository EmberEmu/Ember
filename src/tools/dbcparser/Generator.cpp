/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//#include "Definition.h"
#include "Generator.h"
#include "TypeUtils.h"
#include "Types.h"
#include <regex>
#include <vector>
#include <fstream>
#include <sstream>

namespace ember { namespace dbc {

std::string parent_alias(const std::vector<Definition>& defs, const std::string& parent) {
	for(auto& i : defs) {
		if(i.dbc_name == parent) {
			if(!i.alias.empty()) {
				return i.alias;
			} else {
				return pascal_to_underscore(parent);
			}
		}
	}

	return parent; //couldn't find parent, will just assume the validator caught the problem, if any
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

void generate_linker(const std::vector<Definition>& defs, const std::string& output) {
	std::stringstream buffer(read_template("Linker.cpp_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_LINKING_FUNCTIONS%>([^]+)<%TEMPLATE_LINKING_FUNCTION_CALLS%>([^]+))");
	std::stringstream functions, calls;

	for(auto& i : defs) {
		std::string store_name = i.alias.empty()? pascal_to_underscore(i.dbc_name) : i.alias;
		std::stringstream call, func;
		bool write_func = false;
		bool double_spaced = false;
		bool first_field = true;
		bool pack_loop_format = true;

		call << "\t" << "detail::link_" << store_name << "(storage);" << std::endl;
		
		func << "void link_" << store_name << "(Storage& storage) {" << std::endl;
		func << "\t" << "for(auto& i : storage." << store_name << ") {" << std::endl;

		for(auto& f : i.fields) {
			std::stringstream curr_field;
			auto components = extract_components(f.type);
			bool array = components.second.is_initialized();
			bool write_field = false;

			if(array) {
				curr_field << (pack_loop_format? "" : "\n") << (double_spaced || first_field? "" : "\n")
					<< "\t\t" << "for(std::size_t j = 0; j < "
					<< "sizeof(i." << f.name << ") / sizeof(" << type_map[components.first] << ");"
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
	//std::cout << std::regex_replace(buffer.str(), pattern, replace_pattern);
}

void generate_disk_loader(const std::vector<Definition>& defs, const std::string& output) {
	std::stringstream buffer(read_template("DiskLoader.cpp_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_DISK_LOAD_FUNCTIONS%>([^]+)<%TEMPLATE_DISK_LOAD_FUNCTION_CALLS%>([^]+))");
	std::stringstream functions, calls;

	for(auto& i : defs) {
		std::string store_name = i.alias.empty()? pascal_to_underscore(i.dbc_name) : i.alias;
		bool double_spaced = false;
		calls << "\t" << "detail::load_" << store_name << "(storage, dir_path_);" << std::endl;

		functions << "void load_" << store_name << "(Storage& storage, const std::string& dir_path) {"
			<< std::endl;
		functions << "\t" << "bi::file_mapping file(std::string(dir_path + \"" << i.dbc_name
			<< ".dbc\").c_str(), bi::read_only);" << std::endl;
		functions << "\t" << "bi::mapped_region region(file, bi::read_only);"
			<< std::endl;
		functions << "\t" << "auto dbc = get_offsets<disk::" << i.dbc_name <<
			">(region.get_address());" << std::endl << std::endl;
							
		functions << "\t" << "for(std::uint32_t i = 0; i < dbc.header->records; ++i) {" << std::endl;
		functions << "\t\t" << i.dbc_name << " entry{};" << std::endl;
		
		for(auto& f : i.fields) {
			auto components = extract_components(f.type);
			bool array = components.second.is_initialized();
			bool str_offset = components.first.find("string_ref") != std::string::npos;
			
			if(array) {
				functions << (double_spaced? "" : "\n") << "\t\t"
					<< "for(std::size_t j = 0; j < sizeof(dbc.records[i]."
					<< f.name << " / sizeof(" << type_map[components.first]
					<< ")); ++j) {" << std::endl;
			}

			double_spaced = array;

			functions << (array? "\t" : "") << "\t\t" << "entry." << f.name 
				<< (array? "[j]" : "") << " = " << (str_offset? "dbc.strings + " : "")
				<< "dbc.records[i]." << f.name << (array? "[j]" : "") <<  ";" <<  std::endl;

			if(array) {
				functions << "\t\t" << "}" << std::endl << std::endl;
			}
		}

		functions << "\t\t" << "storage." << store_name << ".emplace_back(entry);" << std::endl;
		functions << "\t" << "}" << std::endl;
		functions << "}" << std::endl << std::endl;
	}

	std::string replace_pattern("$1" + functions.str() + "$2" + calls.str() + "$3");
	//std::cout << std::regex_replace(buffer.str(), pattern, replace_pattern);
}

void generate_disk_defs(const std::vector<Definition>& defs, const std::string& output) {
	std::stringstream buffer(read_template("DiskDefs.h_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_DBC_DEFINITIONS%>([^]+))");
	std::stringstream definitions;

	for(auto& i : defs) {
		definitions << "struct " << i.dbc_name << " {" << std::endl;

		for(auto& f : i.fields) {
			auto components = extract_components(f.type);
			std::string field = type_map[components.first] + " " + f.name;

			if(components.second) {
				field =  field + "[" + std::to_string(*components.second) + "]";
			}

			definitions << "\t" << field << ";" << std::endl;
		}

		definitions << "};" << std::endl << std::endl;
	}

	std::string replace_pattern("$1" + definitions.str() + "$2");
	//std::cout << std::regex_replace(buffer.str(), pattern, replace_pattern);
}

bool is_enum(const std::string& type) {
	return type.find("enum") != std::string::npos;
}

void generate_memory_defs(const std::vector<Definition>& defs, const std::string& output) {
	std::stringstream buffer(read_template("MemoryDefs.h_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_MEMORY_FORWARD_DECL%>([^]+)<%TEMPLATE_MEMORY_DEFINITIONS%>([^]+))");
	std::stringstream forward_decls, definitions;

	for(auto& i : defs) {
		forward_decls << "struct " << i.dbc_name << ";" << std::endl;
		definitions << "struct " << i.dbc_name << " {" << std::endl;

		for(auto& f : i.fields) {
			auto components = extract_components(f.type);
			bool array = components.second.is_initialized();
			bool key = false;

			if(is_enum(components.first)) {
				//definitions << "enum class " << f.
			} else {
				for(auto& k : f.keys) {
					if(k.type == "foreign") {
						definitions << "\t" + k.parent << "* " << f.name
							<< (array? "[" + std::to_string(*components.second) + "]" : "")
							<< ";" << std::endl;
						key = true;
						break;
					}
				}

				definitions << "\t" << type_map[components.first] << " " << f.name << (key? "_id" : "")
					<< (array? "[" + std::to_string(*components.second) + "]" : "") << ";" << std::endl;
			}
		}

		definitions << "};" << std::endl << std::endl;
	}

	std::string replace_pattern("$1" + forward_decls.str() + "$2" + definitions.str() + "$3");
	//std::cout << std::regex_replace(buffer.str(), pattern, replace_pattern);
}

void generate_storage(const std::vector<Definition>& defs, const std::string& output) {
	std::stringstream buffer(read_template("Storage.h_"));
	std::regex pattern(R"(([^]+)<%TEMPLATE_DBC_MAPS%>([^]+)<%TEMPLATE_MOVES%>([^]+))");
	std::stringstream declarations, moves;

	for(auto& i : defs) {
		std::string store_name = i.alias.empty()? pascal_to_underscore(i.dbc_name) : i.alias;
		declarations << "\tDBCMap<" << i.dbc_name << "> " << store_name << ";\n";
		moves << "\t\t" << store_name << " = std::move(" << store_name << ");\n";
	}

	std::string replace_pattern("$1" + declarations.str() + "$2" + moves.str() + "$3");
	//std::cout << std::regex_replace(buffer.str(), pattern, replace_pattern);
}

void generate_common(const std::vector<Definition>& defs, const std::string& output) {
	generate_storage(defs, output);
	generate_memory_defs(defs, output);
	generate_disk_defs(defs, output);
	generate_disk_loader(defs, output);
	generate_linker(defs, output);
}

void generate_disk_source(const std::vector<Definition>& defs, const std::string& output) {

}

}} //dbc, ember