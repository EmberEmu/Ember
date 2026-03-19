/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandHelpers.h"
#include "Prototypes.h"
#include "Library.h"
#include "ServiceRunner.h"
#include <account/Service.h>
#include <banner/Banner.h>
#include <character/Service.h>
#include <realm/Service.h>
#include <login/Service.h>
#include <mdns/Service.h>
#include <world/Service.h>
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
#include <thread>

using namespace ember;
using namespace ember::fusion;
namespace opts = boost::program_options;
using Registries = std::unordered_map<std::string, commands::Registry>;

constexpr cstring_view app_name { "Fusion" };
std::unordered_map<std::string, std::unique_ptr<ServiceRunner>> runners;

opts::variables_map parse_arguments(int, const char*[]);
opts::variables_map load_options(const std::string&, const opts::options_description&);
int launch(const opts::variables_map&, Registries&, bool, log::Logger&);
void stop_services();

template<ServiceIndex idx, auto fn>
std::unique_ptr<ServiceRunner> create_runner(const opts::variables_map& args, Registries& registry,
                                             bool share_logger, log::Logger& logger,
                                             const opts::options_description& opt_descs);

void register_service_commands(commands::Registry& registry, log::Logger& logger);

struct Context {
	bool share_logger;
	Registries* registries;
	opts::variables_map* args;
	log::Logger* logger;
};

std::unique_ptr<Context> context;

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
	register_command_handlers(registries, logger);
	register_shared_commands(registries, logger);
	register_service_commands(registries.at("root"), logger);

	context = std::make_unique<Context>(Context {
		.share_logger = share_logger,
		.registries = &registries,
		.args = &args,
		.logger = &logger
	});

	const auto ret = launch(args, registries, share_logger, logger);
	LOG_INFO_SYNC(logger, "{} terminated", app_name);
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

template<typename func_type>
auto create_dyn_service(const cstring_view lib_name, const cstring_view func_name,
                        log::Logger& logger, commands::Registry& registry) {
	auto library = fusion::library::open(lib_name);

	if(!library) {
		throw std::runtime_error(
			std::format("Unable to load library, {}", func_name)
		);
	}

	auto result = fusion::library::find_symbol<func_type>(*library, func_name);

	if(!result) {
		throw std::runtime_error(
			std::format("Unable to find function, {}", func_name)
		);
	}

	return Wrapped {
		.service = (*result)(logger, registry)
		.lib_handle = library
	};
}

template<ServiceIndex idx, auto fn>
auto create_service(log::Logger& logger, commands::Registry& registry) {
#ifdef BUILD_SHARED_SERVICES
	return create_dyn_service<decltype(fn)>(
		lib_props[idx].libname, lib_props[idx].create_fn, logger, registry
	);
#else
	return fn(logger, registry);
#endif
}

std::unique_ptr<log::Logger> configure_logger(std::string_view name,
                                              opts::variables_map& opts,
                                              bool shared) {
	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string(std::format("[{}]", name));
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	if(!shared) {
		auto service_logger = std::make_unique<log::Logger>();
		utility::configure_logger(*service_logger, opts);
		return service_logger;
	} else {
		return nullptr;
	}
}

log::Logger* log_select(std::unique_ptr<log::Logger>& dynlog, log::Logger& logger) {
	return dynlog? dynlog.get() : &logger;
}

// this genuinely might be the worst bit of code I've ever written 
template<ServiceIndex idx, auto fn>
std::unique_ptr<ServiceRunner> create_runner(const opts::variables_map& args, Registries& registry,
                                             bool share_logger, log::Logger& logger,
                                             const opts::options_description& opt_descs) {
	const std::string name(lib_props[idx].name);
	const auto conf_name = std::format("{}.config", name);
	const auto& conf_path = args[conf_name].as<std::string>();
	auto opts = load_options(conf_path, opt_descs);
	auto logptr = configure_logger(name, opts, share_logger);
	auto use_logger = log_select(logptr, logger);
	auto result = create_service<idx, fn>(*use_logger, registry[name]);
	auto runner = std::make_unique<fusion::ServiceRunner>(result, std::move(opts), *use_logger);
	runner->store_logger(std::move(logptr));
	return runner;
}

std::unique_ptr<ServiceRunner> create_mdns_runner(const opts::variables_map& args,
                                                  Registries& registry,
                                                  bool share_logger,
                                                  log::Logger& logger) {
	return create_runner<service_mdns, ember::dns::create_mdns>(
		args, registry, share_logger, logger, dns::Service::options()
	);
}

std::unique_ptr<ServiceRunner> create_login_runner(const opts::variables_map& args,
                                                   Registries& registry,
                                                   bool share_logger,
                                                   log::Logger& logger) {
	return create_runner<service_login, ember::login::create_login>(
		args, registry, share_logger, logger, login::Service::options()
	);
}

std::unique_ptr<ServiceRunner> create_realm_runner(const opts::variables_map& args,
                                                   Registries& registry,
                                                   bool share_logger,
                                                   log::Logger& logger) {
	return create_runner<service_realm, ember::realm::create_realm>(
		args, registry, share_logger, logger, realm::Service::options()
	);
}

std::unique_ptr<ServiceRunner> create_account_runner(const opts::variables_map& args,
                                                     Registries& registry,
                                                     bool share_logger,
                                                     log::Logger& logger) {
	return create_runner<service_account, ember::account::create_account>(
		args, registry, share_logger, logger, account::Service::options()
	);
}

std::unique_ptr<ServiceRunner> create_character_runner(const opts::variables_map& args,
												       Registries& registry,
												       bool share_logger,
												       log::Logger& logger) {
	return create_runner<service_character, ember::character::create_character>(
		args, registry, share_logger, logger, character::Service::options()
	);
}

std::unique_ptr<ServiceRunner> create_world_runner(const opts::variables_map& args,
												   Registries& registry,
												   bool share_logger,
												   log::Logger& logger) {
	return create_runner<service_world, ember::world::create_world>(
		args, registry, share_logger, logger, world::Service::options()
	);
}

std::unique_ptr<ServiceRunner> create_runner(ServiceIndex service,
											 const opts::variables_map& args,
											 Registries& registry,
											 bool share_logger,
											 log::Logger& logger) {
	switch(service) {
		case ServiceIndex::service_mdns:
			return create_mdns_runner(args, registry, share_logger, logger);
		case ServiceIndex::service_login:
			return create_login_runner(args, registry, share_logger, logger);
		case ServiceIndex::service_realm:
			return create_realm_runner(args, registry, share_logger, logger);
		case ServiceIndex::service_world:
			return create_world_runner(args, registry, share_logger, logger);
		case ServiceIndex::service_account:
			return create_account_runner(args, registry, share_logger, logger);
		case ServiceIndex::service_character:
			return create_character_runner(args, registry, share_logger, logger);
		default:
			std::unreachable();
	}
}

void service_start(const std::string& service, log::Logger& logger) {
	auto idx = string_to_idx(service);

	if(idx == ServiceIndex::service_invalid) {
		LOG_CONSOLE_ERROR_ASYNC(logger, "Unrecognised service name, {}", service);
		return;
	}

	if(auto it = runners.find(service); it != runners.end()) {
		auto& [_, runner] = *it;

		if(!runner->is_stopped()) {
			LOG_CONSOLE_ERROR_ASYNC(logger, "Service is already running", service);
			return;
		}
	}

	auto runner = create_runner(
		idx, *context->args, *context->registries, context->share_logger, *context->logger
	);

	LOG_CONSOLE_ASYNC(logger, "Starting {} service...", service);
	runner->run();
	runners.insert_or_assign(service, std::move(runner));
}

void service_stop(const std::string& service, log::Logger& logger) {
	auto it = runners.find(service);

	if(it == runners.end()) {
		LOG_CONSOLE_ERROR_ASYNC(logger, R"(Service "{}" not found or not running)", service);
		return;
	}

	auto& [_, runner] = *it;

	if(runner->is_stopped()) {
		LOG_CONSOLE_ERROR_ASYNC(logger, "Service is not running", service);
		return;
	}

	LOG_CONSOLE_ASYNC(logger, "Waiting for {} service to stop...", service);
	runner->stop();
	LOG_CONSOLE_ASYNC(logger, "Service stopped");

#ifdef BUILD_SHARED_SERVICES
	library::close(runner->handle().lib_handle);
#endif

}

void service_restart(std::string service, log::Logger& logger) {
	service_stop(service, logger);
	service_start(service, logger);
}

void register_service_commands(commands::Registry& registry, log::Logger& logger) {
	auto svc_cmd = registry.insert("service")
		->description("Commands for service control");

	svc_cmd->insert("start")
		->argument("service", commands::args::Type::at_string)
		->description("Start service (load library if shared)")
		->handler([&](auto args) {
			service_start(std::get<std::string>(args["service"]), logger);
		});

	svc_cmd->insert("stop")
		->argument("service", commands::args::Type::at_string)
		->description("Stop service (unload library if shared)")
		->handler([&logger](auto args) {
			service_stop(std::get<std::string>(args["service"]), logger);
		});

	svc_cmd->insert("restart")
		->argument("service", commands::args::Type::at_string)
		->description("Restart service (reload library if shared)")
		->handler([&logger](auto args) {
			service_restart(std::get<std::string>(args["service"]), logger);
		});
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

	// Start initial specified services
	if(args["mdns.active"].as<bool>()) {
		auto runner = create_runner<service_mdns, ember::dns::create_mdns>(
			args, registry, share_logger, logger, dns::Service::options()
		);

		runners.emplace("mdns", std::move(runner));
	}

	if(args["account.active"].as<bool>()) {
		auto runner = create_runner<service_account, ember::account::create_account>(
			args, registry, share_logger, logger, account::Service::options()
		);

		runners.emplace("account", std::move(runner));
	}

	if(args["character.active"].as<bool>()) {
		auto runner = create_runner<service_character, ember::character::create_character>(
			args, registry, share_logger, logger, character::Service::options()
		);

		runners.emplace("character", std::move(runner));
	}

	if(args["login.active"].as<bool>()) {
		auto runner = create_runner<service_login, ember::login::create_login>(
			args, registry, share_logger, logger, login::Service::options()
		);

		runners.emplace("login", std::move(runner));
	}

	if(args["realm.active"].as<bool>()) {
		auto runner = create_runner<service_login, ember::realm::create_realm>(
			args, registry, share_logger, logger, realm::Service::options()
		);

		runners.emplace("realm", std::move(runner));
	}

	if(args["world.active"].as<bool>()) {
		auto runner = create_runner<service_login, ember::world::create_world>(
			args, registry, share_logger, logger, world::Service::options()
		);

		runners.emplace("world", std::move(runner));
	}

	for(auto& runner : runners | std::views::values) {
		runner->run();
	}

	service.run();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "{}", e.what());
	return EXIT_FAILURE;
}

void stop_services() {
	for(auto& runner : runners | std::views::values) {
		runner->stop();
	}
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
		("mdns.active", opts::value<bool>()->required())
		("mdns.config", opts::value<std::string>()->required())
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