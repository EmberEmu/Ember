/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Connections.h"
#include "../Events.h"
#include <logger/Logger.h>
#include <bprinter/table_printer.h>
#include <string>

namespace ember::realm {

void set_offline(const commands::Arguments& args, RealmService& service,
                 EventDispatcher& dispatcher, log::Logger& logger) {
	service.set_offline();
	LOG_CONSOLE(logger, "Set realm status to offline");

	if(!args.contains("disconnect")) {
		return;
	}

	const bool disconnect = args["disconnect"].as<bool>();

	if(disconnect) {
		const Event event {
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

commands::ScopedCommand add_set_commands(commands::Command& registry, utility::CommandExecutor& exec,
										 EventDispatcher& dispatcher, RealmService& service,
                                         log::Logger& logger) {
	auto root = commands::create("set")
		->description("Commands for realm state configuration");

	root->insert("offline")
		->description("Sets the realm list listing to offline, optionally disconnecting players")
		->argument<bool>("disconnect", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			set_offline(args, service, dispatcher, logger);
		}));

	root->insert("online")
		->description("Sets the realm list listing to online")
		->handler(exec([&](const commands::Arguments& args) {
			set_online(service, logger);
		}));

	// register scoped root 'message' command
	return registry.scoped_insert(root);
}

} // realm, ember