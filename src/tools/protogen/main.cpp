/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Validator.h"
#include "Generator.h"
#include <logger/ConsoleSink.h>
#include <logger/FileSink.h>
#include <logger/Logger.h>
#include <shared/utility/polyfill/print>
#include <boost/program_options.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <vector>

using namespace ember;
namespace opts = boost::program_options;

void launch(const opts::variables_map& args, log::Logger& logger);
opts::variables_map parse_arguments(const int argc, const char* argv[]);
std::vector<std::filesystem::path> collect_message_files(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const std::string& content);
jsoncons::jsonschema::json_schema<jsoncons::json> load_schema(const std::filesystem::path& path);
jsoncons::json load_json(const std::filesystem::path& path);
void configure_logger(log::Logger& logger, const opts::variables_map& args);

int main(int argc, const char* argv[]) try {
	const auto args = parse_arguments(argc, argv);

	log::Logger logger;
	configure_logger(logger, args);

	launch(args, logger);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	std::println(stderr, "Fatal error: {}", e.what());
	return EXIT_FAILURE;
}

void configure_logger(log::Logger& logger, const opts::variables_map& args) {
	const auto con_verbosity = args["verbosity"].as<log::Severity>();
	const auto file_verbosity = args["fverbosity"].as<log::Severity>();

	auto fsink = std::make_shared<log::FileSink>(
		file_verbosity, log::Filter(0), "protogen.log", log::FileSink::Mode::append
	);

	auto consink = std::make_shared<log::ConsoleSink>(con_verbosity, log::Filter(0));
	consink->colourise(true);
	logger->add_sink(std::move(consink));
	logger->add_sink(std::move(fsink));
	log::global_logger(logger);
}

void launch(const opts::variables_map& args, log::Logger& logger) try {
	const auto generate = args.count("output") > 0;
	const auto templates_dir = generate? args["templates"].as<std::filesystem::path>() : std::filesystem::path();
	const auto output_dir = generate? args["output"].as<std::filesystem::path>() : std::filesystem::path();

	const auto message_schema = load_schema(args["schema"].as<std::filesystem::path>());
	const auto types_schema = load_schema(args["types-schema"].as<std::filesystem::path>());

	const auto type_defs = load_json(args["types"].as<std::filesystem::path>());
	types_schema.validate(type_defs);

	const auto registry = protogen::build_registry(type_defs);
	validate_registry_internals(registry);

	const auto message_files = collect_message_files(args["messages"].as<std::filesystem::path>());
	std::vector<std::string> generated_paths;

	for(const auto& file : message_files) {
		const auto json = load_json(file);

		if(json.is_object() && json.empty()) {
			LOG_INFO(logger, "Skipping stub, {}", file.filename().string());
			continue;
		}

		message_schema.validate(json);

		validate_message_fields(
			json["fields"], registry,
			std::format("message '{}'", file.string())
		);

		if(!generate) {
			LOG_INFO(logger, "Successfully validated {}", file.string());
			return;
		}

		auto out = generate_message(json, registry, templates_dir);
		write_file(output_dir / out.relative_path, out.content);
		generated_paths.emplace_back(out.relative_path);
		LOG_INFO(logger, "Generated {}", (output_dir / out.relative_path).filename().string());
	}

	if(generate) { 	// generate a include that contains all packet includes
		std::ranges::sort(generated_paths);
		const auto aggregator = protogen::generate_aggregator(generated_paths, templates_dir);
		const auto path = output_dir / "Packets.h";
		write_file(path, aggregator);
		LOG_INFO(logger, "Generated {}", path.filename().string());
	}

	LOG_INFO(logger, "Zug zug, work complete!");
} catch(const std::exception& e) {
	LOG_FATAL(logger, e.what());
}

jsoncons::jsonschema::json_schema<jsoncons::json> load_schema(const std::filesystem::path& path) {
	auto schema = load_json(path);

	return jsoncons::jsonschema::make_json_schema(schema,
		jsoncons::jsonschema::evaluation_options{}.require_format_validation(true)
	);
}

jsoncons::json load_json(const std::filesystem::path& path) {
	std::ifstream stream(path);

	if(!stream) {
		throw std::invalid_argument(std::format("Unable to read {}", path.string()));
	}

	return jsoncons::json::parse(stream);
}

std::vector<std::filesystem::path> collect_message_files(const std::filesystem::path& path) {
	if(!std::filesystem::is_directory(path)) {
		return { path };
	}

	std::vector<std::filesystem::path> paths;

	for(const auto& entry : std::filesystem::directory_iterator(path)) {
		if(entry.is_regular_file() && entry.path().extension() == ".json") {
			paths.emplace_back(entry.path());
		}
	}

	std::ranges::sort(paths);
	return paths;
}

void write_file(const std::filesystem::path& path, const std::string& content) {
	std::filesystem::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);

	if(!out) {
		throw std::runtime_error(
			std::format("Unable to open {} for writing", path.string())
		);
	}

	out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	opts::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("schema,s", opts::value<std::filesystem::path>()->required(),
			"Path to the message JSON schema")
		("types-schema", opts::value<std::filesystem::path>()->required(),
			"Path to the types JSON schema")
		("types,t", opts::value<std::filesystem::path>()->required(),
			"Path to the type definitions (types.json)")
		("messages,m", opts::value<std::filesystem::path>()->required(),
			"Path to a message definition JSON file or a directory to process")
		("output,o", opts::value<std::filesystem::path>(),
			"Output directory for generated code (if omitted, only validates)")
		("templates", opts::value<std::filesystem::path>(),
			"Directory containing the inja template files (required when --output is set)")
		("verbosity,v", opts::value<log::Severity>()->default_value(log::Severity::info),
			"Logging verbosity")
		("fverbosity", opts::value<log::Severity>()->default_value(log::Severity::disabled),
			"File logging verbosity");

	opts::variables_map options;
	opts::store(opts::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	opts::notify(options);

	if(options.count("output") && !options.count("templates")) {
		throw std::invalid_argument("--templates must be provided when --output is set");
	}

	return options;
}