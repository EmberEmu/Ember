/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandHelpers.h"
#include "CreateHelpers.h"
#include "Prototypes.h"
#include "Library.h"
#include "ServiceContext.h"
#include "ServiceRunner.h"
#include <banner/Banner.h>
#include <logger/Logger.h>
#include <thread/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/LogConfig.h>
#include <shared/utility/Utility.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <utility>

using namespace ember;
using namespace ember::fusion;
namespace opts = boost::program_options;
using Registries = std::unordered_map<std::string, commands::Registry>;

constexpr cstring_view app_name { "Fusion" };
std::unordered_map<std::string, ServiceRunner> runners;

opts::variables_map parse_arguments(int, const char*[]);
int launch(const opts::variables_map&, log::Logger&);
void register_service_commands(commands::Registry& registry, log::Logger& logger);
void stop_services();

Registries registries;
Params glob_params{};

int main(int argc, const char* argv[]) try {
	thread::set_name("Main");
	print_banner(app_name);
	utility::set_window_title(app_name);

	auto args = parse_arguments(argc, argv);
	const bool share_logger = args["console_log.enable_input"].as<bool>();

	if(share_logger) {
		boost::any prefix(std::string(""));
		args.insert_or_assign("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger logger;
	utility::configure_logger(logger, args);
	log::global_logger(logger);

	register_command_handlers(registries, logger);
	register_shared_commands(registries, logger);
	register_service_commands(registries.at("fusion"), logger);

	glob_params = Params {
		.args = &args,
		.logger = &logger,
		.share_logger = share_logger,
	};

	const auto ret = launch(args, logger);
	SLOG_INFO(logger, "{} terminated", app_name);
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

Params create_params(commands::Registry* registry) {
	return Params {
		.registry = registry,
		.args = glob_params.args,
		.logger = glob_params.logger,
		.share_logger = glob_params.share_logger
	};
}

void service_start(const std::string& service, log::Logger& logger) {
	auto idx = string_to_idx(service);

	if(idx == ServiceIndex::service_invalid) {
		LOG_CONERR(logger, "Unrecognised service name, {}", service);
		return;
	}

	if(auto it = runners.find(service); it != runners.end()) {
		auto& [_, runner] = *it;

		if(!runner.is_stopped()) {
			LOG_CONERR(logger, "Service ({}) is already running", service);
			return;
		}
	}

	auto& registry = registries[service];
	const auto params = create_params(&registry);
	auto runner = create_runner(idx, params);

	LOG_CONSOLE(logger, "Starting {} service...", service);
	runner.run();
	runners.insert_or_assign(service, std::move(runner));
}

void service_stop(const std::string& service, log::Logger& logger) {
	auto it = runners.find(service);

	if(it == runners.end()) {
		LOG_CONERR(logger, R"(Service "{}" not found or not running)", service);
		return;
	}

	auto& [_, runner] = *it;

	if(runner.is_stopped()) {
		LOG_CONERR(logger, "Service is not running", service);
		return;
	}

	LOG_CONSOLE(logger, "Waiting for {} service to stop...", service);
	runner.stop();
	LOG_CONSOLE(logger, "Service stopped");

	destroy_service(runner.context());
	runners.erase(it);
}

void service_restart(std::string service, log::Logger& logger) {
	service_stop(service, logger);
	service_start(service, logger);
}

void register_service_commands(commands::Registry& registry, log::Logger& logger) {
	auto svc_cmd = registry.insert("service")
		->description("Commands for service control");

	svc_cmd->insert("start")
		->argument<std::string>("service")
		->description("Start service (load library if shared)")
		->handler([&](const commands::Arguments& args) {
			service_start(args["service"].as<std::string>(), logger);
		});

	svc_cmd->insert("stop")
		->argument<std::string>("service")
		->description("Stop service (unload library if shared)")
		->handler([&logger](const commands::Arguments& args) {
			service_stop(args["service"].as<std::string>(), logger);
		});

	svc_cmd->insert("restart")
		->argument<std::string>("service")
		->description("Restart service (reload library if shared)")
		->handler([&logger](const commands::Arguments& args) {
			service_restart(args["service"].as<std::string>(), logger);
		});
}

int launch(const opts::variables_map& args, log::Logger& logger) try {
	// Start initial specified services
	if(args["mdns.active"].as<bool>()) {
		auto& registry = registries["mdns"];
		const auto params = create_params(&registry);
		auto runner = create_runner(ServiceIndex::service_mdns, params);
		runners.emplace("mdns", std::move(runner));
	}

	if(args["account.active"].as<bool>()) {
		auto& registry = registries["account"];
		const auto params = create_params(&registry);
		auto runner = create_runner(ServiceIndex::service_account, params);
		runners.emplace("account", std::move(runner));
	}

	if(args["character.active"].as<bool>()) {
		auto& registry = registries["character"];
		const auto params = create_params(&registry);
		auto runner = create_runner(ServiceIndex::service_character, params);
		runners.emplace("character", std::move(runner));
	}

	if(args["login.active"].as<bool>()) {
		auto& registry = registries["login"];
		const auto params = create_params(&registry);
		auto runner = create_runner(ServiceIndex::service_login, params);
		runners.emplace("login", std::move(runner));
	}

	if(args["realm.active"].as<bool>()) {
		auto& registry = registries["realm"];
		const auto params = create_params(&registry);
		auto runner = create_runner(ServiceIndex::service_realm, params);
		runners.emplace("realm", std::move(runner));
	}

	if(args["world.active"].as<bool>()) {
		auto& registry = registries["world"];
		const auto params = create_params(&registry);
		auto runner = create_runner(ServiceIndex::service_world, params);
		runners.emplace("world", std::move(runner));
	}

	for(auto& runner : runners | std::views::values) {
		runner.run();
	}

	// start signal handling
	boost::asio::io_context ioc;
	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

	signals.async_wait([&](auto error, auto signal) {
		SLOG_DEBUG(logger, "Received signal {}({})", utility::sig_str(signal), signal);
		signals.clear();
		stop_services();
	});

	ioc.run();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	SLOG_FATAL(logger, e.what());
	return EXIT_FAILURE;
}

void stop_services() {
	for(auto& runner : runners | std::views::values) {
		runner.stop();
	}
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	// Command-line options
	opts::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("config,c", opts::value<std::string>()->default_value("fusion.conf"),
			 "Path to the configuration file");

	opts::positional_options_description pos;
	pos.add("config", 1);

	// Config file options
	opts::options_description config_opts("Fusion configuration options");
	config_opts.add_options()
		("mdns.active", opts::value<bool>()->required())
		("mdns.config", opts::value<std::string>()->required())
		("mdns.prefix", opts::value<std::string>()->required())
		("mdns.file", opts::value<std::string>()->required())
		("account.active", opts::value<bool>()->required())
		("account.config", opts::value<std::string>()->required())
		("account.prefix", opts::value<std::string>()->required())
		("account.file", opts::value<std::string>()->required())
		("character.active", opts::value<bool>()->required())
		("character.config", opts::value<std::string>()->required())
		("character.prefix", opts::value<std::string>()->required())
		("character.file", opts::value<std::string>()->required())
		("realm.active", opts::value<bool>()->required())
		("realm.config", opts::value<std::string>()->required())
		("realm.prefix", opts::value<std::string>()->required())
		("realm.file", opts::value<std::string>()->required())
		("world.active", opts::value<bool>()->required())
		("world.config", opts::value<std::string>()->required())
		("world.prefix", opts::value<std::string>()->required())
		("world.file", opts::value<std::string>()->required())
		("login.active", opts::value<bool>()->required())
		("login.config", opts::value<std::string>()->required())
		("login.prefix", opts::value<std::string>()->required())
		("login.file", opts::value<std::string>()->required())
		("console_log.enable_input", opts::value<bool>()->required())
		("console_log.verbosity", opts::value<log::Severity>()->required())
		("console_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("console_log.colours", opts::value<bool>()->required())
		("console_log.prefix", opts::value<std::string>()->default_value(""))
		("console_log.suggestions", opts::value<bool>()->required())
		("remote_log.verbosity", opts::value<log::Severity>()->required())
		("remote_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", opts::value<std::string>()->required())
		("remote_log.host", opts::value<std::string>()->required())
		("remote_log.port", opts::value<std::uint16_t>()->required())
		("file_log.verbosity", opts::value<log::Severity>()->required())
		("file_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("file_log.path", opts::value<std::string>()->default_value("fusion.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::value<bool>()->required())
		("file_log.log_timestamp", opts::value<bool>()->required())
		("file_log.log_severity", opts::value<bool>()->required());

	opts::variables_map options;
	opts::store(opts::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	opts::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	const auto& config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		throw std::invalid_argument("Unable to open configuration file: " + config_path);
	}

	opts::store(opts::parse_config_file(ifs, config_opts), options);
	opts::notify(options);

	return options;
}