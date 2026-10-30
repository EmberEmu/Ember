/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Prototypes.h"
#include "ServiceRunner.h"
#include <account/Service.h>
#include <character/Service.h>
#include <realm/Service.h>
#include <login/Service.h>
#include <mdns/Service.h>
#include <world/Service.h>
#include <commands/Commands.h>
#include <logger/Logger.h>
#include <logger/CommandSink.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/LogConfig.h>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <fstream>
#include <utility>

namespace ember::fusion {

namespace opts = boost::program_options;

struct Params {
	std::shared_ptr<commands::Command> registry;
	opts::variables_map* args;
	log::Logger* logger;
};

template<typename func_type>
auto locate_symbol(const library::Handle lib, const cstring_view func_name) {
	auto result = fusion::library::find_symbol<func_type>(lib, func_name);

	if(!result) {
		throw std::runtime_error(
			std::format("Unable to find function, {}", func_name)
		);
	}

	return *result;
}

template<typename func_type>
auto locate_symbol(const cstring_view lib_name, const cstring_view func_name) {
	auto library = fusion::library::open(lib_name);

	if(!library) {
		throw std::runtime_error(
			std::format("Unable to load library, {}", func_name)
		);
	}

	auto symbol = fusion::library::find_symbol<func_type>(*library, func_name);

	if(!symbol) {
		throw std::runtime_error(
			std::format("Unable to find function, {}", func_name)
		);
	}

	return std::pair(*library, *symbol);
}

template<auto fn>
auto create_service(ServiceIndex idx, log::Logger& logger, commands::Command& registry) {
	library::Handle handle = nullptr;
	Service* service = nullptr;

#ifdef BUILD_SHARED_SERVICES
	auto [library, create_func] = locate_symbol<decltype(fn)>(
		lib_props[idx].libname, lib_props[idx].create_fn
	);

	handle = library;
	service = create_func(logger, registry);
#else
	service = fn(logger, registry);
#endif

	return ServiceContext {
		.service = service,
		.lib_handle = handle,
		.index = idx
	};
}

template<auto fn, typename derived_type>
auto destroy_service(const ServiceContext& context) {
	auto derived = static_cast<derived_type*>(context.service);

#ifdef BUILD_SHARED_SERVICES
	auto destroy_fn = locate_symbol<decltype(fn)>(
		context.lib_handle, lib_props[context.index].destroy_fn
	);

	destroy_fn(derived);
#else
	fn(derived);
#endif
}

/*
 * It is possible to obviate these switches but it'd require modifying the exported
 * create/destroy functions from each service to return/accept the abstract service type
 * rather than the derived type. Cleaner code but also increases the risk of accidentally
 * calling into the wrong shared library very slightly. Might be worth it since I'm not
 * sure if the switch statements are really any less error-prone.
 */
inline void destroy_service(const ServiceContext& context) {
	switch(context.index) {
		case ServiceIndex::service_account:
			destroy_service<account::destroy_account, account::Service>(context);
			break;
		case ServiceIndex::service_world:
			destroy_service<world::destroy_world, world::Service>(context);
			break;
		case ServiceIndex::service_character:
			destroy_service<character::destroy_character, character::Service>(context);
			break;
		case ServiceIndex::service_realm:
			destroy_service<realm::destroy_realm, realm::Service>(context);
			break;
		case ServiceIndex::service_login:
			destroy_service<login::destroy_login, login::Service>(context);
			break;
		case ServiceIndex::service_mdns:
			destroy_service<dns::destroy_mdns, dns::Service>(context);
			break;
		default:
			std::abort();
	}

#ifdef BUILD_SHARED_SERVICES
	library::close(context.lib_handle);
#endif
}

inline std::unique_ptr<log::Logger> configure_logger(const std::string_view name, const Params& params) {
	opts::variables_map opts = *params.args;
	
	{ // give this service its own log file, if file logging is enabled
		const auto option = std::format("{}.file", name);
		opts.insert_or_assign("file_log.path", opts.at(option));
	}

	std::unique_ptr<log::Logger> logger;

	if(opts["console_log.prefixes"].as<bool>()) {
		const auto option = std::format("{}.prefix", name);
		opts.insert_or_assign("console_log.prefix", opts.at(option));

		logger = std::make_unique<log::Logger>(
			opts["console_log.prefix"].as<std::string>()
		);
	} else {
		logger = std::make_unique<log::Logger>();
	}

	// suppress per-logger command sink creation
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(false, false));

	// we're going to share the command sink (input handling) between all logger instances within this process
	auto cmd_sink = params.logger->fetch_unique(log::CommandSink::type());

	if(cmd_sink) {
		logger->add_sink(cmd_sink);

		// suppress creation of any console output sink if there's an active command sink
		opts.insert_or_assign(
			"console_log.verbosity", opts::variable_value(log::Severity::disabled, false)
		);
	}

	// allow for the creation of the remaining sinks
	utility::configure_logger(*logger, opts);
	return logger;
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

inline void print_defaulted(const opts::variables_map& args, log::Logger& logger) {
	for(const auto& arg : args) {
		const auto& [key, value] = arg;

		if(value.defaulted()) {
			SLOG_WARN(logger, "Configuration key '{}' missing, using default value", key);
		}
	}
}

// this genuinely might be the worst bit of code I've ever written 
template<auto fn>
ServiceRunner create_runner(const ServiceIndex idx, const Params& params,
                            const opts::options_description& descs) {
	// determine the path to the config file for this service and load it
	const auto conf_section = std::format("{}.config", lib_props[idx].name);
	const auto& conf_path = params.args->at(conf_section).as<std::string>();
	auto opts = load_options(conf_path, descs);

	auto service_logger = configure_logger(lib_props[idx].name, params);

	// finally, create the service and assign it to a runner wrapper (coroutines when?)
	auto service = create_service<fn>(idx, *service_logger, *params.registry);
	service.logger = std::move(service_logger);
	print_defaulted(opts, *service.logger);
	return ServiceRunner(std::move(service), std::move(opts));
}

inline ServiceRunner create_mdns_runner(const Params& params) {
	return create_runner<dns::create_mdns>(
		ServiceIndex::service_mdns, params, dns::Service::options()
	);
}

inline ServiceRunner create_login_runner(const Params& params) {
	return create_runner<login::create_login>(
		ServiceIndex::service_login, params, login::Service::options()
	);
}

inline ServiceRunner create_realm_runner(const Params& params) {
	return create_runner<realm::create_realm>(
		ServiceIndex::service_realm, params, realm::Service::options()
	);
}

inline ServiceRunner create_account_runner(const Params& params) {
	return create_runner<account::create_account>(
		ServiceIndex::service_account, params, account::Service::options()
	);
}

inline ServiceRunner create_character_runner(const Params& params) {
	return create_runner<character::create_character>(
		ServiceIndex::service_character, params, character::Service::options()
	);
}

inline ServiceRunner create_world_runner(const Params& params) {
	return create_runner<world::create_world>(
		ServiceIndex::service_world, params, world::Service::options()
	);
}

inline ServiceRunner create_runner(ServiceIndex service, const Params& params) {
	switch(service) {
		case ServiceIndex::service_mdns:
			return create_mdns_runner(params);
		case ServiceIndex::service_login:
			return create_login_runner(params);
		case ServiceIndex::service_realm:
			return create_realm_runner(params);
		case ServiceIndex::service_world:
			return create_world_runner(params);
		case ServiceIndex::service_account:
			return create_account_runner(params);
		case ServiceIndex::service_character:
			return create_character_runner(params);
		default:
			std::unreachable();
	}
}

} // fusion, ember