/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PluginControl.h"
#include "../Common.h"
#include "../ServiceContextImpl.h"
#include <commands/Commands.h>
#include <logger/Logger.h>
#include <string>
#include <string_view>

namespace ember::blaze {

void list_plugins(ServiceContext& context, log::Logger& logger) {
	const auto plugins = context.get()->plugins.get();
	
	for(auto& plugin : plugins->all()) {
		LOG_CONSOLE(logger, "Plugin name: {} ({})", plugin->name(), plugin->pid());
	}
}

void reload_plugin(ServiceContext& context, log::Logger& logger, const PluginID pid) {
	const auto plugins = context.get()->plugins.get();
	auto plugin = plugins->locate(pid);

	if(!plugin) {
		LOG_CONERR(logger, "Unable to locate plugin #{}", pid);
		return;
	}

	LOG_CONERR(logger, "Command not yet implemented");
}

void load_plugin(ServiceContext& context, log::Logger& logger, const std::string_view path) {
	LOG_CONERR(logger, "Command not yet implemented");
}

void unload_plugin(ServiceContext& context, log::Logger& logger, const PluginID pid) {
	const auto plugins = context.get()->plugins.get();
	auto plugin = plugins->locate(pid);

	if(!plugin) {
		LOG_CONERR(logger, "Unable to locate plugin #{}", pid);
		return;
	}

	LOG_CONERR(logger, "Command not yet implemented");
}

void install_plugin_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto root = commands::create("plugins")
		->description("Subcommands for plugin management");

	root->insert("list")
		->description("Display all loaded plugins")
		->handler(/*exec/*/([&](const auto&) {
			list_plugins(context, logger);
		}));

	root->insert("load")
		->description("Loads a plugin from the given path")
		->argument<std::string>("path")
		->handler(/*exec/*/([&](const commands::Arguments& args) {
			load_plugin(context, logger, args["path"].as<std::string_view>());
		}));

	root->insert("reload")
		->description("Reloads a plugin")
		->argument<PluginID>("plugin_id")
		->handler(/*exec/*/([&](const commands::Arguments& args) {
			reload_plugin(context, logger, args["plugin_id"].as<PluginID>());
		}));


	root->insert("unload")
		->description("Unloads a plugin")
		->argument<PluginID>("plugin_id")
		->handler(/*exec/*/([&](const commands::Arguments& args) {
			unload_plugin(context, logger, args["plugin_id"].as<PluginID>());
		}));

	registry.insert(root);
}

} // blaze, ember