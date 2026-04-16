/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Message.h"
#include "Connections.h"
#include "../Events.h"
#include "../ServiceContextImpl.h"
#include <logger/Logger.h>
#include <bprinter/table_printer.h>
#include <string>

namespace ember::realm {

void system_message(const commands::Arguments& args,
                    const SessionManager& sessions,
					const EventDispatcher& dispatcher,
                    log::Logger& logger,
                    SystemMessage::Type type) {
	const auto& message = args["message"].as<std::string>();

	if(args.contains("id")) {
		const auto id = args["id"].as<SessionManager::SessionID>();
		auto ident = sessions.client_ident(id);

		if(!ident) {
			LOG_CONERR(logger, "Could not locate connection {}", id);
			return;
		}

		SystemMessage event(message, type);
		dispatcher.post(*ident, std::move(event));
		LOG_CONSOLE(logger, "Message sent to connection {}", id);
	} else {
		SystemMessage event(message, type);
		dispatcher.broadcast(std::move(event));
		LOG_CONSOLE(logger, "Message sent to all connections");
	}
}

void add_message_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();
	auto& exec = *impl->cmd_exec;
	auto& sessions = impl->sessions;
	auto& dispatcher = *impl->dispatcher;

	auto root = commands::create("message")
		->description("Broadcast system messages or message players");

	root->insert("notify")
		->description("Send a system notification to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const auto& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::console);
		}));

	root->insert("chat")
		->description("Send a system chat message to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const auto& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::message);
		}));

	root->insert("whisper")
		->description("Send a system whisper to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const auto& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::whisper);
		}));

	auto scoped = registry.scoped_insert(root);
	impl->commands.emplace_back(std::move(scoped));
}

} // realm, ember