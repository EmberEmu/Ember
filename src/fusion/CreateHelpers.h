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
#include <commands/Registry.h>
#include <logger/Logger.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/LogConfig.h>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <fstream>

namespace ember::fusion {

namespace opts = boost::program_options;

struct Params {
	commands::Registry* registry;
	opts::variables_map* args;
	log::Logger* logger;
	bool share_logger;
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

	auto result = fusion::library::find_symbol<func_type>(*library, func_name);

	if(!result) {
		throw std::runtime_error(
			std::format("Unable to find function, {}", func_name)
		);
	}

	return std::pair(*library, *result);
}

template<auto fn>
auto create_service(ServiceIndex idx, log::Logger& logger, commands::Registry& registry) {
	library::Handle handle = nullptr;
	IService* service = nullptr;

#ifdef BUILD_SHARED_SERVICES
	auto [library, create_func] = locate_symbol<decltype(fn)>(
		lib_props[idx].libname, lib_props[idx].create_fn
	);

	handle = library;
	service = (create_func)(logger, registry);
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
	auto destroy_fn = locate_symbol<decltype(fn)>(context.lib_handle, lib_props[idx].destroy_fn);
	(destroy_fn)(derived);
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

inline std::unique_ptr<log::Logger> configure_logger(const std::string_view name, opts::variables_map& opts) {
	if(!opts.contains("console_log.prefix")) {
		boost::any prefix = std::string(std::format("[{}]", name));
		opts.try_emplace("console_log.prefix", opts::variable_value(prefix, false));
	}

	// disable console input option
	opts.insert_or_assign("console_log.enable_input", opts::variable_value(boost::any(false), false));

	auto service_logger = std::make_unique<log::Logger>();
	utility::configure_logger(*service_logger, opts);
	return service_logger;
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

// this genuinely might be the worst bit of code I've ever written 
template<auto fn>
ServiceRunner create_runner(const ServiceIndex idx, Params& params, const opts::options_description& descs) {
	// determine the path to the config file for this service and load it
	const auto conf_section = std::format("{}.config", lib_props[idx].name);
	const auto& conf_path = params.args->at(conf_section).as<std::string>();
	auto opts = load_options(conf_path, descs);

	// if we're allowing the service to create its own logger, set it up
	std::unique_ptr<log::Logger> service_logger;
	log::Logger* logger_handle = params.logger;

	if(!params.share_logger) {
		service_logger = configure_logger(lib_props[idx].name, opts);
		logger_handle = service_logger.get();
	}

	// finally, create the service and assign it to a runner wrapper (coroutines when?)
	auto service = create_service<fn>(idx, *logger_handle, *params.registry);
	service.logger = std::move(service_logger);
	return ServiceRunner(std::move(service), std::move(opts));
}

inline ServiceRunner create_mdns_runner(Params& params) {
	return create_runner<dns::create_mdns>(
		ServiceIndex::service_mdns, params, dns::Service::options()
	);
}

inline ServiceRunner create_login_runner(Params& params) {
	return create_runner<login::create_login>(
		ServiceIndex::service_login, params, login::Service::options()
	);
}

inline ServiceRunner create_realm_runner(Params& params) {
	return create_runner<realm::create_realm>(
		ServiceIndex::service_realm, params, realm::Service::options()
	);
}

inline ServiceRunner create_account_runner(Params& params) {
	return create_runner<account::create_account>(
		ServiceIndex::service_account, params, account::Service::options()
	);
}

inline ServiceRunner create_character_runner(Params& params) {
	return create_runner<character::create_character>(
		ServiceIndex::service_character, params, character::Service::options()
	);
}

inline ServiceRunner create_world_runner(Params& params) {
	return create_runner<world::create_world>(
		ServiceIndex::service_world, params, world::Service::options()
	);
}

inline ServiceRunner create_runner(ServiceIndex service, Params& params) {
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