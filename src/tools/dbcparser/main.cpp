/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include "Generator.h"
#include <spark/BinaryStream.h>
#include <spark/buffers/ChainedBuffer.h>
#include "bprinter/table_printer.h"
#include <boost/filesystem.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>

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

const std::unordered_map<std::string, int> type_size_map {
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

class RecordPrinter : public types::TypeVisitor {
	ember::spark::ChainedBuffer<4096> buffer_, sb_buffer_;
	ember::spark::BinaryStream record_, string_block_;
	const std::string default_string = "Hello, world!";

	void generate_field(const std::string& type) {
		if(type == "int8" || type == "uint8" || type == "bool") {
			record_ << std::uint8_t(1);
		} else if(type == "int16" || type == "uint16") {
			record_ << std::uint16_t(1);
		} else if(type == "int32" || type == "uint32" || type == "bool32") {
			record_ << std::uint32_t(1);
		} else if(type == "float") {
			record_ << 1.0f;
		} else if(type == "double") {
			record_ << 1.0;
		} else if(type == "string_ref") {
			record_ << std::uint32_t(string_block_.size());
			string_block_ << default_string;
		} else if(type == "string_ref_loc") {
			for(int i = 0; i < 9; ++i) {
				record_ << std::uint32_t(string_block_.size());
				string_block_ << default_string;
			}
		}
	}

public:
	RecordPrinter() : record_(buffer_), string_block_(sb_buffer_) {
		// DBC editors choke if the block doesn't start with 0 with the first string at offset 1
		// This is apparently just DBC editors being bad, which is why this tool exists to begin with
		string_block_ << std::uint8_t(0);
	}

	void visit(const types::Struct* type) override {
		// we don't care about structs
	}

	void visit(const types::Enum* type) override {
		generate_field(type->underlying_type);
	}

	void visit(const types::Field* type) override {
		auto components = extract_components(type->underlying_type);
		int scalar_size = type_size_map.at(components.first);
		std::size_t elements = 1;

		// if this is an array, we need to write multiple records
		if(components.second) {
			elements = *components.second;
		}

		for(std::size_t i = 0; i < elements; ++i) {
			generate_field(components.first);
		}
	}

	std::vector<char> string_block() {
		std::vector<char> data(string_block_.size());
		string_block_.get(data.data(), data.size());
		return data;
	}

	std::vector<char> record() {
		std::vector<char> data(record_.size());
		record_.get(data.data(), data.size());
		return data;
	}
};

class TypeMetrics : public types::TypeVisitor {
public:
	int fields = 0;
	int size = 0;

	void visit(const types::Struct* type) override {
		// we don't care about structs
	}

	void visit(const types::Enum* type) override {
		++fields;
		size += type_size_map.at(type->underlying_type);
	}

	void visit(const types::Field* type) override {
		auto components = extract_components(type->underlying_type);
		int scalar_size = type_size_map.at(components.first);
		
		// handle arrays
		if(components.second) {
			fields += *components.second;
			size += (scalar_size * (*components.second));
		} else {
			++fields;
			size += scalar_size;
		}
	}
};

template<typename T>
void walk_dbc_fields(const types::Struct* dbc, T& visitor) {
	for(auto f : dbc->fields) {
		std::string type = f.underlying_type;

		// if this is a user-defined struct, we need to go through that type too
		// if it's an enum, we can just grab the underlying type
		auto components = extract_components(f.underlying_type);
		auto it = type_map.find(components.first);

		if(it != type_map.end()) {
			visitor.visit(&f);
		} else {
			auto found = locate_type(*dbc, f.underlying_type);

			if(!found) {
				throw std::runtime_error("Unknown field type encountered, " + f.underlying_type);
			}

			if(found->type == types::STRUCT) {
				walk_dbc_fields(static_cast<types::Struct*>(found), visitor);
			} else if(found->type == types::ENUM) {
				visitor.visit(static_cast<types::Enum*>(found));
			}
		}
	}
}

namespace be = boost::endian;

void generate_template(const std::string& dbc, const edbc::types::Definitions& groups) {
	auto def = locate_dbc(dbc, groups);

	if(!def) {
		throw std::invalid_argument(dbc + " - no such DBC");
	}
	std::ofstream file(def->name + ".dbc", std::ofstream::binary);
	
	TypeMetrics metrics;
	walk_dbc_fields(def, metrics);

	RecordPrinter printer;
	walk_dbc_fields(def, printer);

	std::vector<char> record_data = printer.record();
	std::vector<char> string_data = printer.string_block();

	be::big_uint32_t magic('WDBC');
	be::little_uint32_t records = 1;
	be::little_uint32_t fields = metrics.fields;
	be::little_uint32_t size = metrics.size;
	be::little_uint32_t string_block_size = string_data.size();
	
	// write header
	file.write((char*)&magic, sizeof(magic));
	file.write((char*)&records, sizeof(records));
	file.write((char*)&fields, sizeof(fields));
	file.write((char*)&size, sizeof(size));
	file.write((char*)&string_block_size, sizeof(string_block_size));
	
	// write dummy record
	file.write(record_data.data(), record_data.size());

	// write string block
	file.write(string_data.data(), string_data.size());
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