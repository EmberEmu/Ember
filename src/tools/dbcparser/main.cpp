/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <boost/program_options.hpp>
#include <string>

namespace po = boost::program_options;

po::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	const po::variables_map arguments = parse_arguments(argc, argv);
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}


po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description opt("Generic options");
	opt.add_options()
		("help", "Displays a list of available options")
		("definitions,d", po::value<std::string>()->default_value("definitions"),
			"Path to the DBC XML definitions"))
		("headers,h", po::bool_switch()->default_value(false),
			"Generate header files"))
		("sql-schema,s", po::bool_switch()->default_value(false),
			"Generate SQL data"))
		("sql-inserts,i", po::bool_switch()->default_value(false),
			"Generate SQL data"))

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(opt).run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << opt << "\n";
		std::exit(0);
	}

	return std::move(options);
}