/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Parser.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <string>
#include <vector>
#include <iostream>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace edbc = ember::dbc;

po::variables_map parse_arguments(int argc, const char* argv[]);
std::vector<std::string> fetch_definitions(const std::string& path);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	const std::string def_path = args["definitions"].as<std::string>();
	std::vector<std::string> paths = fetch_definitions(def_path);
	
	edbc::Parser parser;

	for(auto& p : paths) {
		parser.add_definition(p);
	}

	std::vector<edbc::Definition> definitions = parser.finish();

} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}


std::vector<std::string> fetch_definitions(const std::string& path)
{
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
		("help", "Displays a list of available options")
		("definitions,d", po::value<std::string>()->default_value("definitions"),
			"Path to the DBC XML definitions")
		("headers,h", po::bool_switch()->default_value(false),
			"Generate header files")
		("sql-schema,s", po::bool_switch()->default_value(false),
			"Generate SQL data")
		("sql-inserts,i", po::bool_switch()->default_value(false),
			"Generate SQL data");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(opt).run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << opt << "\n";
		std::exit(0);
	}

	return std::move(options);
}