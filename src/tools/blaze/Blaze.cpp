/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Blaze.h"
#include "Extensions.h"
#include "InterfaceContainer.h"
#include <thread/Utility.h>
#include <filesystem>
#include <thread>
#include "Library.h"

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
		LOG_ERROR(logger_, "Unable to load plugin, {}: {}",
			path.filename().string(), library::result_to_string(handle.error()));
		return;
	}

	const auto symbol = library::find_symbol<const char**>(*handle, "plugin_name");

	if(!symbol) {
		LOG_ERROR(logger_, "Unable to load plugin, {}: {}",
			path.filename().string(), library::result_to_string(symbol.error()));
		library::close(*handle);
		return;
	}

	plugins_.emplace_back(*handle, **symbol);
	LOG_INFO(logger_, "Loaded plugin, {}", **symbol);
}

} // blaze, ember