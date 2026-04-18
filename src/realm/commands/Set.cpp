/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Set.h"
#include "../Events.h"
#include "../ServiceContextImpl.h"
#include <logger/Logger.h>

namespace ember::realm {

namespace {

void set_offline(const commands::Arguments& args, RealmService& service,
                 EventDispatcher& dispatcher, log::Logger& logger) {
	service.set_offline();
	LOG_CONSOLE(logger, "Set realm status to offline");

	if(!args.contains("kick")) {
		return;
	}

	if(args["kick"].as<bool>()) {
		const Event event{
			.type = EventType::kick_self
		};

		// allows for a graceful close
		dispatcher.broadcast(event);
	}
}

void set_online(RealmService& service, log::Logger& logger) {
	service.set_online();
	LOG_CONSOLE(logger, "Set realm status to online");
}

} // unnamed

void add_set_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();
	auto& exec = *impl->cmd_exec;
	auto& dispatcher = *impl->dispatcher;
	auto& rpc_realm = *impl->rpc_realm;

	auto root = commands::create("set")
		->description("Commands for realm state configuration");

	root->insert("offline")
		->description("Sets the realm list listing to offline, optionally kicking players")
		->argument<bool>("kick", commands::optional)
		->handler(exec([&](const auto& args) {
			set_offline(args, rpc_realm, dispatcher, logger);
		}));

	root->insert("online")
		->description("Sets the realm list listing to online")
		->handler(exec([&](const auto& args) {
			set_online(rpc_realm, logger);
		}));

	auto scoped = registry.scoped_insert(root);
	impl->commands.emplace_back(std::move(scoped));
}

} // realm, ember