/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StreamReader.h"
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

namespace po = boost::program_options;

namespace ember {

void launch(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);

} // ember

int main(int argc, const char* argv[]) try {
	using namespace ember;

	const po::variables_map args = parse_arguments(argc, argv);
	launch(args);
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

namespace ember {

void launch(const po::variables_map& args) {
	std::ifstream file(args.at("file").as<std::string>(),
		std::ifstream::in | std::ifstream::binary);

	if(!file) {
		throw std::runtime_error("Unable to open packet dump file");
	}

	const auto interval = std::chrono::seconds(args.at("interval").as<unsigned int>());
	const auto stream = args.at("stream").as<bool>();
	auto skip = stream? args.at("skip").as<bool>() : false;

	StreamReader reader(file, stream, skip, interval);
	reader.process();
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description opt("Options");

	opt.add_options()
		("help,h", "Displays a list of available options")
		("file,f", po::value<std::string>()->default_value(""),
			"Path to packet capture dump file")
		("stream,s", po::bool_switch(),
			"Treat the input as a stream, monitoring for any new packets")
		("skip,k", po::bool_switch(),
			"If treating the packet dump as a stream, skip output of existing packets")
		("interval,i", po::value<unsigned int>()->default_value(2),
			"Frequency for checking the file stream for new packets")
		("format", po::value<std::string>(),
			"Todo")
		 ("filter", po::value<std::string>(),
		  "Todo");

	po::positional_options_description pos; 
	pos.add("file", 1);

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).positional(pos).options(opt)
	          .style(po::command_line_style::default_style & ~po::command_line_style::allow_guessing)
	          .run(), options);
	po::notify(options);

	if(options.count("help") || argc <= 1) {
		std::cout << opt << "\n";
		std::exit(0);
	}

	return options;
}

} // ember