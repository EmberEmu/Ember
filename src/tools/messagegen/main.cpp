/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/utility/polyfill/print>
#include <boost/program_options.hpp>
#include <stdexcept>
#include <iostream>

namespace po = boost::program_options;

void launch(const boost::program_options::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	const auto args = parse_arguments(argc, argv);
	launch(args);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

void launch(const boost::program_options::variables_map& args) {

}

po::variables_map parse_arguments(const int argc, const char* argv[]) {
	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	po::notify(options);
	return options;
}