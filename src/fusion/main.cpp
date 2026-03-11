/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandHelpers.h"
#include <account/Service.h>
#include <banner/Banner.h>
#include <character/Service.h>
#include <realm/Service.h>
#include <login/Service.h>
#include <mdns/Service.h>
#include <world/Service.h>
#include <logger/Logger.h>
#include <shared/utility/cstring_view.hpp>
#include <thread/Utility.h>
#include <shared/utility/LogConfig.h>
#include <shared/utility/Utility.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <atomic>
#include <fstream>
#include <functional>
#include <iostream>
#include <span>
#include <thread>
#include <vector>

constexpr ember::cstring_view app_name { "Fusion" };

namespace opts = boost::program_options;

using namespace ember;
using Registries = std::unordered_map<std::string, commands::Registry>;

opts::variables_map parse_arguments(int, const char*[]);
opts::variables_map load_options(const std::string&, const opts::options_description&);
int launch(const opts::variables_map&, Registries&, bool, log::Logger&);
void launch_dns(const opts::variables_map&, commands::Registry&, bool, log::Logger&);
void launch_login(const opts::variables_map&, commands::Registry&, bool, log::Logger&);
void launch_realm(const opts::variables_map&, commands::Registry&, bool, log::Logger&);
void launch_account(const opts::variables_map&, commands::Registry&, bool, log::Logger&);
void launch_character(const opts::variables_map&, commands::Registry&, bool, log::Logger&);
void launch_world(const opts::variables_map&, commands::Registry&, bool, log::Logger&);
void stop_services();

std::vector<std::function<void()>> stop_handlers;
std::atomic_bool shutting_down = false;


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

	Registries registries;
	fusion::register_command_handlers(registries, logger);
	fusion::register_shared_commands(registries, logger);

	const auto ret = launch(args, registries, share_logger, logger);
	LOG_INFO_SYNC(logger, "{} terminated", app_name);
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int launch(const opts::variables_map& args, Registries& registry, bool share_logger, log::Logger& logger) try {
	boost::asio::io_context service;
	boost::asio::signal_set signals(service, SIGINT, SIGTERM);

	signals.async_wait([&](auto error, auto signal) {
		LOG_DEBUG_SYNC(logger, "Received signal {}({})", utility::sig_str(signal), signal);
		signals.clear();
		stop_services();
	});

	std::jthread worker([&]() {
		thread::set_name("Signal handler");
		service.run_one();
	});

	// Start services
	std::vector<std::jthread> services;

	if(args["dns.active"].as<bool>()) {
		services.emplace_back(std::jthread([&]() {
			launch_dns(args, registry["dns"], share_logger, logger);
		}));
	}

	if(args["account.active"].as<bool>()) {
		services.emplace_back(std::jthread([&]() {
			launch_account(args, registry["account"], share_logger, logger);
		}));
	}

	if(args["character.active"].as<bool>()) {
		services.emplace_back(std::jthread([&]() {
			launch_character(args, registry["character"], share_logger, logger);
		}));
	}

	if(args["login.active"].as<bool>()) {
		services.emplace_back(std::jthread([&]() {
			launch_login(args, registry["login"], share_logger, logger);
		}));
	}

	if(args["realm.active"].as<bool>()) {
		services.emplace_back(std::jthread([&]() {
			launch_realm(args, registry["realm"], share_logger, logger);
		}));
	}

	if(args["world.active"].as<bool>()) {
		services.emplace_back(std::jthread([&]() {
			launch_world(args, registry["world"], share_logger, logger);
		}));
	}

	if(services.empty()) {
		LOG_INFO_SYNC(logger, "No services specified? Nothing to do, farewell.");
	}

	// explicitly join so we can cancel the signal afterwards
	for(auto& threads : services) {
		threads.join();
	}

	signals.cancel();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "{}", e.what());
	return EXIT_FAILURE;
}

void stop_services() {
	shutting_down = true;

	for(auto& stop : stop_handlers) {
		stop();
	}
}

void launch_dns(const opts::variables_map& args, commands::Registry& registry, bool share_logger, log::Logger& logger) try {
	LOG_INFO_SYNC(logger, "Starting DNS service...");

	const auto& conf_path = args["dns.config"].as<std::string>();
	auto opts = load_options(conf_path, dns::Service::options());

	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string("[mdns]");
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger service_logger;
	log::Logger* active_logger = &service_logger;

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!share_logger) {
		utility::configure_logger(*active_logger, opts);
	} else {
		active_logger = &logger;
	}

	dns::Service service(*active_logger, registry);

	stop_handlers.emplace_back([&] {
		service.stop();
	});

	const auto res = service.run(opts);

	if(res != EXIT_SUCCESS || !shutting_down) {
		LOG_FATAL_SYNC(logger, "DNS service terminated abnormally or unexpectedly, aborting");
		std::exit(res);
	}
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "DNS error: {}", e.what());
	std::exit(EXIT_FAILURE);
}

void launch_login(const opts::variables_map& args, commands::Registry& registry, bool share_logger, log::Logger& logger) try {
	LOG_INFO_SYNC(logger, "Starting login service...");

	const auto& conf_path = args["login.config"].as<std::string>();
	auto opts = load_options(conf_path, login::Service::options());

	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string("[login]");
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger service_logger;
	log::Logger* active_logger = &service_logger;

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!share_logger) {
		utility::configure_logger(*active_logger, opts);
	} else {
		active_logger = &logger;
	}

	login::Service service(*active_logger, registry);

	stop_handlers.emplace_back([&] {
		service.stop();
	});

	const auto res = service.run(opts);

	if(res != EXIT_SUCCESS || !shutting_down) {
		LOG_FATAL_SYNC(logger, "Login service terminated abnormally or unexpectedly, aborting");
		std::exit(res);
	}
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "Login error: {}", e.what());
	std::exit(EXIT_FAILURE);
}

void launch_realm(const opts::variables_map& args, commands::Registry& registry, bool share_logger, log::Logger& logger) try {
	LOG_INFO_SYNC(logger, "Starting realm service...");

	const auto& conf_path = args["realm.config"].as<std::string>();
	auto opts = load_options(conf_path, realm::Service::options());

	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string("[realm]");
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger service_logger;
	log::Logger* active_logger = &service_logger;

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!share_logger) {
		utility::configure_logger(service_logger, opts);
	} else {
		active_logger = &logger;
	}

	realm::Service service(*active_logger, registry);

	stop_handlers.emplace_back([&] {
		service.stop();
	});

	const auto res = service.run(opts);

	if(res != EXIT_SUCCESS || !shutting_down) {
		LOG_FATAL_SYNC(logger, "Realm terminated abnormally or unexpectedly, aborting");
		std::exit(res);
	}
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "Realm error: {}", e.what());
	std::exit(EXIT_FAILURE);
}

void launch_account(const opts::variables_map& args, commands::Registry& registry, bool share_logger, log::Logger& logger) try {
	LOG_INFO_SYNC(logger, "Starting account service...");

	const auto& conf_path = args["account.config"].as<std::string>();
	auto opts = load_options(conf_path, account::Service::options());

	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string("[account]");
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger service_logger;
	log::Logger* active_logger = &service_logger;

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!share_logger) {
		utility::configure_logger(service_logger, opts);
	} else {
		active_logger = &logger;
	}

	account::Service service(*active_logger, registry);

	stop_handlers.emplace_back([&] {
		service.stop();
	});

	const auto res = service.run(opts);

	if(res != EXIT_SUCCESS || !shutting_down) {
		LOG_FATAL_SYNC(logger, "Account service terminated abnormally or unexpectedly, aborting");
		std::exit(res);
	}
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "Account service error: {}", e.what());
	std::exit(EXIT_FAILURE);
}

void launch_character(const opts::variables_map& args, commands::Registry& registry, bool share_logger, log::Logger& logger) try {
	LOG_INFO_SYNC(logger, "Starting character service...");

	const auto& conf_path = args["character.config"].as<std::string>();
	auto opts = load_options(conf_path, character::Service::options());

	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string("[character]");
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger service_logger;
	log::Logger* active_logger = &service_logger;

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!share_logger) {
		utility::configure_logger(*active_logger, opts);
	} else {
		active_logger = &logger;
	}

	character::Service service(*active_logger, registry);

	stop_handlers.emplace_back([&] {
		service.stop();
	});

	const auto res = service.run(opts);

	if(res != EXIT_SUCCESS || !shutting_down) {
		LOG_FATAL_SYNC(logger, "Character service terminated abnormally or unexpectedly, aborting");
		std::exit(res);
	}
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "Character error: {}", e.what());
	std::exit(EXIT_FAILURE);
}

void launch_world(const opts::variables_map& args, commands::Registry& registry, bool share_logger, log::Logger& logger) try {
	LOG_INFO_SYNC(logger, "Starting world service...");

	const auto& conf_path = args["world.config"].as<std::string>();
	auto opts = load_options(conf_path, world::Service::options());

	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string("[world]");
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	log::Logger service_logger;
	log::Logger* active_logger = &service_logger;

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!share_logger) {
		utility::configure_logger(*active_logger, opts);
	} else {
		active_logger = &logger;
	}

	world::Service service(*active_logger, registry);

	stop_handlers.emplace_back([&] {
		service.stop();
	});

	const auto res = service.run(opts);

	if(res != EXIT_SUCCESS || !shutting_down) {
		LOG_FATAL_SYNC(logger, "World service terminated abnormally or unexpectedly, aborting");
		std::exit(res);
	}
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "World error: {}", e.what());
	std::exit(EXIT_FAILURE);
}

opts::variables_map load_options(const std::string& config_path, const opts::options_description& opt_desc) {
	std::ifstream ifs(config_path);

	if(!ifs) {
		throw std::invalid_argument("Unable to open configuration file: " + config_path);
	}

	opts::variables_map options;
	opts::store(opts::parse_config_file(ifs, opt_desc, true), options);
	opts::notify(options);

	return options;
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
		("dns.active", opts::value<bool>()->required())
		("dns.config", opts::value<std::string>()->required())
		("account.active", opts::value<bool>()->required())
		("account.config", opts::value<std::string>()->required())
		("character.active", opts::value<bool>()->required())
		("character.config", opts::value<std::string>()->required())
		("realm.active", opts::value<bool>()->required())
		("realm.config", opts::value<std::string>()->required())
		("world.active", opts::value<bool>()->required())
		("world.config", opts::value<std::string>()->required())
		("login.active", opts::value<bool>()->required())
		("login.config", opts::value<std::string>()->required())
		("console_log.enable_input", opts::value<bool>()->required())
		("console_log.verbosity", opts::value<log::Severity>()->required())
		("console_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("console_log.colours", opts::value<bool>()->required())
		("console_log.prefix", opts::value<std::string>()->default_value(""))
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