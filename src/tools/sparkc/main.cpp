/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SchemaParser.h"
#include <logger/Logging.h>
#include <logger/ConsoleSink.h>
#include <logger/FileSink.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

namespace po = boost::program_options;
namespace el = ember::log;

int launch(const po::variables_map& args);
void process_schema(const std::string& schema);
void init_logger(ember::log::Logger* logger, const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	auto logger = std::make_unique<el::Logger>();
	init_logger(logger.get(), args);
	return launch(args);
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int launch(const po::variables_map& args) try {
	const auto output = args["out"].as<std::string>();

	for(const auto& schema : args["schemas"].as<std::vector<std::string>>()) {
		LOG_INFO_GLOB << "Processing " << schema << LOG_SYNC;
		process_schema(schema);
	}

	return EXIT_SUCCESS;
} catch (const std::exception& e) {
	LOG_FATAL_GLOB << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

void process_schema(const std::string& schema) {
	std::vector<std::uint8_t> buffer;
	std::ifstream file(schema, std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error("Unable to open binary schema (bfbs)");
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	buffer.resize(static_cast<std::size_t>(size));

	if (!file.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
		throw std::runtime_error("Unable to read binary schema (bfbs)");
	}

	ember::SchemaParser parser(buffer);
}

void init_logger(ember::log::Logger* logger, const po::variables_map& args) {
	const auto con_verbosity = el::severity_string(args["verbosity"].as<std::string>());
	const auto file_verbosity = el::severity_string(args["fverbosity"].as<std::string>());
	
	auto fsink = std::make_unique<el::FileSink>(
		file_verbosity, el::Filter(0), "sparkc.log", el::FileSink::Mode::APPEND
	);

	auto consink = std::make_unique<el::ConsoleSink>(con_verbosity, el::Filter(0));
	consink->colourise(true);
	logger->add_sink(std::move(consink));
	logger->add_sink(std::move(fsink));
	el::set_global_logger(logger);
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("schemas,s", po::value<std::vector<std::string>>()->multitoken()->required(), ".fbsb schemas")
		("out,o",     po::value<std::string>()->required(), "Output directory for generated code")
		("verbosity,v", po::value<std::string>()->default_value("info"),
			"Logging verbosity")
		("fverbosity", po::value<std::string>()->default_value("disabled"),
			"File logging verbosity");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help") || argc <= 1) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	po::notify(options);

	return options;
}