/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Session.h"
#include "../Events.h"
#include <logger/Logger.h>
#include <bprinter/table_printer.h>
#include <memory>
#include <sstream>

namespace ember::realm {

void system_message(const commands::Arguments& args, SessionManager& sessions,
                    EventDispatcher& dispatcher, log::Logger& logger, bool whisper) {
	const auto& message = args["message"].as<std::string>();

	if(args.contains("id")) {
		SystemMessage event(message, whisper);
		const auto id = args["id"].as<SessionManager::SessionID>();
		auto ident = sessions.client_ident(id);

		if(!ident) {
			LOG_CONERR(logger, "Could not locate connection {}", id);
			return;
		}

		dispatcher.post_event(*ident, std::move(event));
		LOG_CONSOLE(logger, "Message sent to connection {}", id);
	} else {
		auto event = std::make_shared<SystemMessage>(message, whisper);
		dispatcher.broadcast_event(event);
		LOG_CONSOLE(logger, "Message sent to all connections");
	}
}

void toggle_logging(SessionManager::SessionID id, bool toggle,
                    SessionManager& sessions, EventDispatcher& dispatcher,
                    log::Logger& logger) {
	auto ident = sessions.client_ident(id);

	if(!ident) {
		LOG_CONERR(logger, "Could not locate connection {}", id);
		return;
	}

	const auto type = toggle? EventType::packet_log_enable : EventType::packet_log_disable;

	const auto event = Event {
		.type = type
	};

	dispatcher.post_event(*ident, event);
	LOG_CONSOLE(logger, "Packet logging {} for connection {}", (toggle? "enabled" : "disabled"), id);
}

void kick_connection(SessionManager::SessionID id, SessionManager& sessions,
                     EventDispatcher& dispatcher, log::Logger& logger) {
	auto ident = sessions.client_ident(id);

	if(!ident) {
		LOG_CONERR(logger, "Could not locate connection {}", id);
		return;
	}

	const auto event = Event {
		.type = EventType::kick_self
	};

	dispatcher.post_event(*ident, event);
	LOG_CONSOLE(logger, "Connection {} kicked", id);
}

void connection_list(const SessionManager& sessions, log::Logger& logger) {
	if(!sessions.count()) {
		LOG_CONSOLE(logger, "No connected sessions to display");
		return;
	}

	for(const auto& session : sessions) {
	}

	std::stringstream stream;
	bprinter::TablePrinter table(&stream);
	table.AddColumn("ID", 5);
	table.AddColumn("Remote", 22);
	table.AddColumn("Identity", 25);
	table.AddColumn("State", 25);
	table.PrintHeader();

	std::size_t connections = 0;

	for(const auto& session : sessions) {
		const auto& [k, v] = session;
		table << k;
		table << v->connection().remote_address();
		table << v->handler().client_identify();
		table << ClientState_to_string(v->handler().state());
		++connections;
	}

	table.PrintFooter();
	LOG_CONSOLE(logger, "Displaying {} connections\n{}", connections, stream.str());
}

void connection_statistics(const SessionManager& sessions, log::Logger& logger) {
	if(!sessions.count()) {
		LOG_CONSOLE(logger, "No connected sessions to display");
		return;
	}

	std::stringstream stream;
	bprinter::TablePrinter table(&stream);
	table.AddColumn("ID", 5);
	table.AddColumn("Remote", 22);
	table.AddColumn("Ping", 5);
	table.AddColumn("Msg In", 6);
	table.AddColumn("Msg Out", 7);
	table.AddColumn("Bytes In", 8);
	table.AddColumn("Bytes Out", 9);
	table.AddColumn("Ops In", 6);
	table.AddColumn("Opts Out", 8);
	table.PrintHeader();
	
	std::size_t connections = 0;

	for(const auto& session : sessions) {
		const auto& [k, v] = session;
		const auto stats = v->connection().stats();
		table << k;
		table << v->connection().remote_address();
		table << stats.latency;
		table << stats.messages_in;
		table << stats.messages_out;
		table << stats.bytes_in;
		table << stats.bytes_out;
		table << stats.async_receives;
		table << stats.async_sends;
		++connections;
	}

	table.PrintFooter();
	LOG_CONSOLE(logger, "Displaying statistics for {} connections\n{}", connections, stream.str());
}

commands::ScopedCommand add_session_commands(commands::Command& registry,
                                             utility::CommandExecutor& exec,
                                             EventDispatcher& dispatcher,
                                             SessionManager& sessions,
                                             log::Logger& logger) {

	auto root = commands::create("connections")
		->description("Commands for connection/session management");

	root->insert(commands::create("list")
		->description("List all active connections")
		->handler(exec([&](const commands::Arguments& arguments) {
			connection_list(sessions, logger);
		})));

	root->insert(commands::create("kick")
		->description("Kick a specified connection")
		->argument<SessionManager::SessionID>("id")
		->handler(exec([&](const commands::Arguments& arguments) {
			kick_connection(
				arguments["id"].as<SessionManager::SessionID>(), sessions, dispatcher, logger
			);
		})));

	root->insert(commands::create("netstat")
		->description("Display network stats for all connections")
		->handler(exec([&](const commands::Arguments& arguments) {
			connection_statistics(sessions, logger);
		})));

	root->insert(commands::create("log")
		->description("Enable or disable connection packet logging")
		->argument<SessionManager::SessionID>("id")
		->argument<bool>("bool")
		->handler(exec([&](const commands::Arguments& arguments) {
			toggle_logging(
				arguments["id"].as<SessionManager::SessionID>(), 
				arguments["bool"].as<bool>(), sessions, dispatcher, logger
			);
		})));

	root->insert(commands::create("message")
		->description("Send a system message to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& arguments) {
			system_message(arguments, sessions, dispatcher, logger, false);
		})));

	root->insert(commands::create("whisper")
		->description("Send a system whisper to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& arguments) {
			system_message(arguments, sessions, dispatcher, logger, true);
		})));

	// register scoped root 'connections' command
	return registry.scoped_insert(root);
}

} // ream, ember