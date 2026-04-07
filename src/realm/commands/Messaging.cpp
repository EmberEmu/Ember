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
#include <memory>
#include <string>

namespace ember::realm {

void system_message(const commands::Arguments& args,
                    const SessionManager& sessions,
					const EventDispatcher& dispatcher,
                    log::Logger& logger,
                    SystemMessage::Type type) {
	const auto& message = args["message"].as<std::string>();

	if(args.contains("id")) {
		SystemMessage event(message, type);
		const auto id = args["id"].as<SessionManager::SessionID>();
		auto ident = sessions.client_ident(id);

		if(!ident) {
			LOG_CONERR(logger, "Could not locate connection {}", id);
			return;
		}

		dispatcher.post_event(*ident, std::move(event));
		LOG_CONSOLE(logger, "Message sent to connection {}", id);
	} else {
		auto event = std::make_shared<SystemMessage>(message, type);
		dispatcher.broadcast_event(std::move(event));
		LOG_CONSOLE(logger, "Message sent to all connections");
	}
}

commands::ScopedCommand add_message_commands(commands::Command& registry,
                                             utility::CommandExecutor& exec,
                                             const EventDispatcher& dispatcher,
                                             const SessionManager& sessions,
                                             log::Logger& logger) {
	auto root = commands::create("message")
		->description("Commands for messaging players");

	root->insert("notify")
		->description("Send a system notification to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::console);
		}));

	root->insert("chat")
		->description("Send a system chat message to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::message);
		}));

	root->insert("whisper")
		->description("Send a system whisper to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::whisper);
		}));

	// register scoped root 'message' command
	return registry.scoped_insert(root);
}

} // realm, ember