/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/utility/polyfill/print>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <filesystem>
#include <stdexcept>

void launch(const boost::program_options::variables_map& args);
boost::program_options::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	const auto args = parse_arguments(argc, argv);
	launch(args);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

void launch(const boost::program_options::variables_map& args) {
	auto schema_path = args["schema"].as<std::filesystem::path>();
	std::ifstream stream(schema_path);

	if(!stream) {
		std::println("Unable to open schema, {}", schema_path.string());
		return;
	}

	auto json = jsoncons::json::parse(stream, [](const jsoncons::json_errc ec, const jsoncons::ser_context& context) {
		std::println("Error while parsing schema, error on line {}", context.line());
		return false;
	});

	auto schema = jsoncons::jsonschema::make_json_schema(json);
	auto msgs_path = args["messages"].as<std::filesystem::path>();

	stream.close();
	stream.open(msgs_path);

	if(!stream) {
		std::println("Unable to open test message, {}", msgs_path.string());
		return;
	}

	auto reporter = [&](const jsoncons::jsonschema::validation_message& message) {
		std::println("Error parsing {}: {}:{}", msgs_path.string(), message.instance_location().string(), message.message());
		return jsoncons::jsonschema::walk_result::abort;
	};

	json = jsoncons::json::parse(stream, [](const jsoncons::json_errc ec, const jsoncons::ser_context& context) {
		std::println("Error while parsing message, error on line {}", context.line());
		return false;
	});

	schema.validate(json, reporter);
}

boost::program_options::variables_map parse_arguments(const int argc, const char* argv[]) {
	namespace po = boost::program_options;

	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("schema,s", po::value<std::filesystem::path>()->required(),
			"Path to the JSON schema file to use for message definition validation")
		("messages,m", po::value<std::filesystem::path>()->required(),
			"Path to the directory containing the JSON message definitions to process")
		("verbose,v", po::value<bool>()->default_value(false),
			"Print verbose output for debugging purposes");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	po::notify(options);
	return options;
}