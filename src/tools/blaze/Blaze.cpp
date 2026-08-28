/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Blaze.h"
#include "Common.h"
#include "Extensions.h"
#include "InterfaceContainer.h"
#include "Library.h"
#include "api/Logging.h"
#include <thread/Utility.h>
#include <filesystem>
#include <thread>

namespace ember::blaze {

Blaze::Blaze(const boost::program_options::variables_map& args, commands::Command& registry, log::Logger& logger)
	: args_(args)
	, registry_(registry)
	, logger_(logger) {
	// todo, it's all temporary
	auto interfaces = InterfaceContainer::get_instance();
	interfaces.command_root(&registry);
	interfaces.logger(&logger);
	auto pcr = new PluginCommandRegistry(); // todo, temp!
	interfaces.plugin_command_registry(pcr);
	load_plugins();
}

int Blaze::run() try {
	auto concurrency = args_["misc.threads"].as<unsigned int>();

	if(!concurrency) {
		concurrency = thread::hardware_concurrency([&](auto msg) {
			SLOG_ERROR(logger_, msg);
		});
	}

	boost::asio::io_context ioc(concurrency);
	start_services(ioc);

	std::vector<std::jthread> threads;
	threads.reserve(concurrency);

	for(unsigned int i = 1; i < concurrency; ++i) {
		threads.emplace_back(&boost::asio::io_context::run, &ioc);
		thread::set_name(threads[i], "Asio Worker");
	}

	ioc.run();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	SLOG_FATAL(logger_, e.what());
	return EXIT_FAILURE;
}

void Blaze::start_services(boost::asio::io_context& ioc) {

}

void Blaze::load_plugins() {
	const auto& plugins_path = args_["plugins.path"].as<std::string>();

	if(!std::filesystem::is_directory(plugins_path)) {
		throw std::runtime_error("Unable to open specified plugins path");
	}

	for(auto& file : std::filesystem::directory_iterator(plugins_path)) {
		if(file.path().extension() == SHARED_LIBRARY_EXT) {
			load_plugin(file);
		}
	}
}

void Blaze::load_plugin(const std::filesystem::path& path) {
	auto handle = library::open(path.string());

	if(!handle) {
		LOG_ERROR(logger_, "Unable to load plugin {}: {}",
			path.filename().string(), library::result_to_string(handle.error()));
		return;
	}

	const auto symbol = library::find_symbol<const char**>(*handle, "plugin_name");

	if(!symbol) {
		LOG_ERROR(logger_, "Unable to load plugin {}: {}",
			path.filename().string(), library::result_to_string(symbol.error()));
		library::close(*handle);
		return;
	}

	Plugin plugin(*handle, **symbol);

	plugins_.emplace_back(*handle, **symbol);
	LOG_TRACE(logger_, "Loading plugin {}", plugin.name());

	// more test gubbins
	using PluginLoadFn = void(*)();
	using PluginUnloadFn = void(*)();
	const auto load_fn = library::find_symbol<PluginLoadFn>(*handle, "plugin_on_load");
	const auto unload_fn = library::find_symbol<PluginUnloadFn>(*handle, "plugin_on_unload");

	if(!load_fn || !unload_fn) {
		LOG_ERROR(logger_, "Unable to load plugin {}: {}",
			path.filename().string(), library::result_to_string(symbol.error()));
		return;
	}

	const auto fn = library::find_symbol<sdk_build_fn>(*handle, "sdk_build");

	if(!fn) {
		LOG_ERROR(logger_, "Unable to load plugin {}: {}",
			path.filename().string(), library::result_to_string(symbol.error()));
		return;
	}

	const auto result = (*fn)();

	if(result.magic != SDK_MAGIC) {
		LOG_ERROR(logger_, "Unable to load plugin {}: incorrect magic", plugin.name());
		return;
	}

	if(result.sdk_init_meta_size != sizeof(SDKBuildMeta)) {
		LOG_ERROR(logger_, "Unable to load plugin {}: incompatible metadata structures", plugin.name());
		return;
	}

	if(result.blaze_host_api_size != sizeof(BlazeHostAPI)) {
		LOG_ERROR(logger_, "Unable to load plugin {}: incompatible API structures", plugin.name());
		return;
	}

	if(result.version_major < SDK_MAJOR_VERSION) {
		LOG_ERROR(logger_, "Unable to load plugin {}: plugin is out of date", plugin.name());
		return;
	}

	if(result.version_major > SDK_MAJOR_VERSION) {
		LOG_ERROR(logger_, "Unable to load plugin {}: plugin may be too new", plugin.name());
		return;
	}

	const auto init_fn = library::find_symbol<sdk_initialise_fn>(*handle, "sdk_initialise");

	BlazeHostAPI api {
		.size = sizeof(BlazeHostAPI),
		.version_major = SDK_MAJOR_VERSION,
		.version_minor = SDK_MINOR_VERSION,
		.version_patch = SDK_PATCH_VERSION,
		.log_async = log_async,
		.log_sync = log_sync
	};

	if(const auto result = (*init_fn)(api); result != SDK_INIT_OK) {
		LOG_ERROR(logger_, "Unable to load plugin {}: plugin returned error code {}",
			plugin.name(), result);
		return;
	}

	LOG_INFO(logger_, "{} plugin loaded", plugin.name());
	(*load_fn)();
}

} // blaze, ember