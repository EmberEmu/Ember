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

	if(!args.contains("disconnect")) {
		return;
	}

	const bool disconnect = args["disconnect"].as<bool>();

	if(disconnect) {
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

	auto root = commands::create("set")
		->description("Commands for realm state configuration");

	root->insert("offline")
		->description("Sets the realm list listing to offline, optionally disconnecting players")
		->argument<bool>("disconnect", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			set_offline(args, *impl->rpc_realm, *impl->dispatcher, logger);
		}));

	root->insert("online")
		->description("Sets the realm list listing to online")
		->handler(exec([&](const commands::Arguments& args) {
			set_online(*impl->rpc_realm, logger);
		}));

	auto scoped = registry.scoped_insert(root);
	impl->commands.emplace_back(std::move(scoped));
}

} // realm, ember