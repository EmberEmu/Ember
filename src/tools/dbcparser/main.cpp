/*
 * Copyright (c) 2014, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include "Generator.h"
#include "DBCGenerator.h"
#include "bprinter/table_printer.h"
#include <logger/Logging.h>
#include <logger/ConsoleSink.h>
#include <logger/FileSink.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace edbc = ember::dbc;
namespace el = ember::log;

int launch(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);
std::vector<std::string> fetch_definitions(const std::vector<std::string>& paths);
void print_dbc_table(const edbc::types::Definitions& defs);
void print_dbc_fields(const std::string& dbc, const edbc::types::Definitions& defs);
void handle_options(const po::variables_map& args, const edbc::types::Definitions& defs);
edbc::types::Struct* locate_dbc(const std::string& dbc, const edbc::types::Definitions& defs);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	auto con_verbosity = el::severity_string(args["verbosity"].as<std::string>());
	auto file_verbosity = el::severity_string(args["fverbosity"].as<std::string>());

	auto logger = std::make_unique<el::Logger>();
	auto fsink = std::make_unique<el::FileSink>(file_verbosity, el::Filter(0),
	                                            "dbcparser.log", el::FileSink::Mode::APPEND);
	auto consink = std::make_unique<el::ConsoleSink>(con_verbosity, el::Filter(0));
	consink->colourise(true);
	logger->add_sink(std::move(consink));
	logger->add_sink(std::move(fsink));
	el::set_global_logger(logger.get());

	return launch(args);
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

int launch(const po::variables_map& args) try {
	const auto def_paths = args["definitions"].as<std::vector<std::string>>();

	std::vector<std::string> paths = fetch_definitions(def_paths);

	edbc::Parser parser;
	auto definitions = parser.parse(paths);
	handle_options(args, definitions);
	return 0;
} catch(std::exception& e) {
	LOG_FATAL_GLOB << e.what() << LOG_SYNC;
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
		auto dbc = locate_dbc(args["template"].as<std::string>(), defs);

		if(dbc == nullptr) {
			throw std::invalid_argument("Could not find the specified DBC definition");
		}

		edbc::generate_template(dbc);
		return;
	}

	if(args["disk"].as<bool>() || args.count("database")) {
		edbc::generate_common(defs, args["output"].as<std::string>());
	}


	if(args["disk"].as<bool>()) {
		edbc::generate_disk_source(defs, args["output"].as<std::string>());
	}

	LOG_DEBUG_GLOB << "Done!" << LOG_ASYNC;
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

std::vector<std::string> fetch_definitions(const std::vector<std::string>& paths) {
	std::vector<std::string> xml_paths;

	for(auto& path : paths) {
		if(!boost::filesystem::is_directory(path)) {
			throw std::invalid_argument("Invalid path provided, " + path);
		}

		for(auto& file : fs::directory_iterator(path)) {
			if(file.path().extension() == ".xml") {
				xml_paths.emplace_back(file.path().string());
			}
		}
	}

	return xml_paths;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description opt("Generic options");
	opt.add_options()
		("help,h", "Displays a list of available options")
		("definitions,d", po::value<std::vector<std::string>>()->multitoken(),
			"Path to the DBC XML definitions")
		("output,o", po::value<std::string>()->default_value("output"),
			"Directory to save output to")
		("verbosity,v", po::value<std::string>()->default_value("info"),
		 "Logging verbosity")
		("fverbosity", po::value<std::string>()->default_value("disabled"),
		 "File logging verbosity")
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