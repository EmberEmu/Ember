/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include "Generator.h"
#include "bprinter/table_printer.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace edbc = ember::dbc;

po::variables_map parse_arguments(int argc, const char* argv[]);
std::vector<std::string> fetch_definitions(const std::string& path);
void print_dbc_table(const edbc::types::Definitions& defs);
void print_dbc_fields(const std::string& dbc, const edbc::types::Definitions& defs);
void handle_options(const po::variables_map& args, const edbc::types::Definitions& defs);
void generate_template(const std::string& dbc, const edbc::types::Definitions& groups);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	const std::string def_path = args["definitions"].as<std::string>();
	std::vector<std::string> paths = fetch_definitions(def_path);
	
	edbc::Parser parser;
	auto definitions = parser.parse(paths);

	handle_options(args, definitions);
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void handle_options(const po::variables_map& args, const edbc::types::Definitions& defs) {
	if(args["print-dbcs"].as<bool>()) {
		print_dbc_table(defs);
		return;
	}

	if(args.count("print-fields")) {
		print_dbc_fields(args["print-fields"].as<std::string>(), defs);
		return;
	}


	if(args.count("template")) {
		generate_template(args["template"].as<std::string>(), defs);
		std::cout << "DBC template generated." << std::endl;
		return;
	}

	if(args["disk"].as<bool>() || args.count("database")) {
		std::cout << "Generating shared files...\n";
		edbc::generate_common(defs, args["output"].as<std::string>());
		std::cout << "Common files generated." << std::endl;
	}


	if(args["disk"].as<bool>()) {
		edbc::generate_disk_source(defs, args["output"].as<std::string>());
		std::cout << "Disk loader generated." << std::endl;
	}

	std::cout << "Done!" << std::endl;
}

void print_dbc_table(const edbc::types::Definitions& defs) {
	bprinter::TablePrinter printer(&std::cout);
	printer.AddColumn("DBC Name", 26);
	printer.AddColumn("#", 4);
	printer.AddColumn("Comment", 45);
	printer.PrintHeader();

	for(auto& def : defs) {
		if(def->type == edbc::types::STRUCT) {
			auto dbc = static_cast<edbc::types::Struct*>(def.get());
			printer << dbc->name.substr(0, 26) << dbc->fields.size() << dbc->comment;
		}
	}
}

edbc::types::Struct* locate_dbc(const std::string& dbc, const edbc::types::Definitions& defs) {
	for(auto& def : defs) {
		if(def->name == dbc) {
			if(def->type == edbc::types::STRUCT) {
				return static_cast<edbc::types::Struct*>(def.get());
			}
		}
	}

	return nullptr;
}

#include <boost/optional.hpp>
#include "TypeUtils.h"

using namespace ember::dbc;


types::Base* locate_type(const types::Struct& base, const std::string& type_name) {
	for(auto& f : base.children) {
		if(f->name == type_name) {
			return f.get();
		}
	}

	if(base.parent == nullptr) {
		return nullptr;
	}

	return locate_type(static_cast<types::Struct&>(*base.parent), type_name);
}

#include <unordered_map>

std::unordered_map<std::string, int> size_map {
	{ "int8",           1 },
	{ "uint8",          1 },
	{ "int16",          2 },
	{ "uint16",         2 },
	{ "int32",          4 },
	{ "uint32",         4 },
	{ "bool",           1 },
	{ "bool32",         4 },
	{ "string_ref",     4 },
	{ "string_ref_loc", 36 },
	{ "float",          4 },
	{ "double",         8 }
};

struct TypeMetrics {
	int records;
	int size;
};

TypeMetrics type_metrics(const types::Struct& base, TypeMetrics metrics = {}) {
	for(auto& f : base.fields) {
		std::string type = f.underlying_type;

		// if this is a user-defined struct, we need to go through that type too
		// if it's an enum, we can just grab the underlying type
		auto it = type_map.find(f.underlying_type);

		if(it != type_map.end()) {
			metrics.records += 1;
			metrics.size += size_map.at(f.underlying_type);
		} else {
			auto found = locate_type(base, f.underlying_type);

			if(!found) {
				throw std::runtime_error("Unknown field type encountered, " + f.underlying_type);
			}

			if(found->type == types::STRUCT) {
				metrics = type_metrics(static_cast<types::Struct&>(*found), metrics);
			} else if(found->type == types::ENUM) {
				metrics.records += 1;
				metrics.size += size_map.at(static_cast<types::Enum*>(found)->underlying_type);
			}
		}
	}

	return metrics;
}

void generate_template(const std::string& dbc, const edbc::types::Definitions& groups) {
	auto def = locate_dbc(dbc, groups);

	if(!def) {
		throw std::invalid_argument(dbc + " - no such DBC");
	}

	std::ofstream file(def->name + ".dbc", std::ofstream::binary);
	
	// write header
	std::uint32_t magic('CBDW');
	std::uint32_t records = 1; // one record
	std::uint32_t field_count = 0;
	std::uint32_t string_block_size = 0;
	std::uint32_t record_size = 0;

	for(auto& f : def->fields) {
		if(f.underlying_type == "string_ref_loc") {
			field_count += 9;
			string_block_size += 10;
		} else {
			field_count += 1;
		}
	}
	
	TypeMetrics metrics = type_metrics(*def);

	std::cout << "Size: " << metrics.size << " Records: " << metrics.records << "\n";
	
	std::stringstream string_block;

	const std::string block = string_block.str();
	string_block_size = block.size();

	// not caring about endianness
	file.write((char*)&magic, sizeof(magic));
	file.write((char*)&records, sizeof(records));
	file.write((char*)&field_count, sizeof(field_count));
	file.write((char*)&record_size, sizeof(record_size));
	file.write((char*)&string_block_size, sizeof(string_block_size));
	file.write(block.c_str(), block.size());

	// write an example record
	for(auto& f : def->fields) {
		if(f.underlying_type == "int8" || f.underlying_type == "uint8" || f.underlying_type == "bool") {
			std::uint8_t val(1);
			file.write((char*)&val, 1);
		} else if(f.underlying_type == "int16" || f.underlying_type == "uint16") {
			std::uint16_t val(1);
			file.write((char*)&val, 2);
		} else if(f.underlying_type == "int32" || f.underlying_type == "uint32"
				  || f.underlying_type == "bool32" || f.underlying_type == "string_ref") {
			std::uint32_t val(1);
			file.write((char*)&val, 4);
		} else if(f.underlying_type == "float") {
			float val(1.0f);
			file.write((char*)&val, 4);
		} else if(f.underlying_type == "double") {
			double val(1);
			file.write((char*)&val, 8);
		} else if(f.underlying_type == "string_ref") {
			string_block << "Test string";
			// ??
		} else if(f.underlying_type == "string_ref_loc") {
			string_block << "Test string GB";
			string_block << std::uint32_t(0) << std::uint32_t(0);
		}
	}
}

void print_dbc_fields(const std::string& dbc, const edbc::types::Definitions& groups) {
	auto def = locate_dbc(dbc, groups);
	
	if(!def) {
		throw std::invalid_argument(dbc + " - no such definition to print");
	}

	bprinter::TablePrinter printer(&std::cout);
	printer.AddColumn("Field", 32);
	printer.AddColumn("Type", 18);
	printer.AddColumn("Key", 4);
	printer.AddColumn("Comment", 20);
	printer.PrintHeader();

	for(auto& f : def->fields) {
		std::string key;

		switch(f.keys.size()) {
			case 1:
				key = f.keys[0].type.data()[0];
				break;
			case 2:
				key = "pf";
				break;
		}

		printer << f.name << f.underlying_type << key << f.comment;
	}
}

std::vector<std::string> fetch_definitions(const std::string& path) {
	if(!boost::filesystem::is_directory(path)) {
		throw std::exception("Invalid path provided.");
	}

	std::vector<std::string> paths;

	for(auto& file : fs::directory_iterator(path)) {
		if(file.path().extension() == ".xml") {
			paths.emplace_back(file.path().string());
		}
	}

	return paths;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description opt("Generic options");
	opt.add_options()
		("help,h", "Displays a list of available options")
		("definitions,d", po::value<std::string>()->default_value("definitions"),
			"Path to the DBC XML definitions")
		("output,o", po::value<std::string>()->default_value("output"),
			"Directory to save output to")
		("disk", po::bool_switch(),
			"Generate files required for loading DBC data from disk")
		("print-dbcs", po::bool_switch(),
			"Print out a summary of the DBC definitions in a table")
		("print-fields", po::value<std::string>(),
			"Print out of a summary of a specific DBC definition's fields")
		("template", po::value<std::string>(),
		 "Generate a DBC template file for editing in other tools");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(opt)
	          .style(po::command_line_style::default_style & ~po::command_line_style::allow_guessing)
	          .run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << opt << "\n";
		std::exit(0);
	}

	return std::move(options);
}