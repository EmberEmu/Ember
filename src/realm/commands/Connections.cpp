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
#include <sstream>

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
		dispatcher.broadcast_event(event);
		LOG_CONSOLE(logger, "Message sent to all connections");
	}
}

void toggle_logging(SessionManager::SessionID id,
                    bool toggle,
					const SessionManager& sessions,
					const EventDispatcher& dispatcher,
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

void kick_connection(SessionManager::SessionID id,
					 const SessionManager& sessions,
					 const EventDispatcher& dispatcher,
                     log::Logger& logger) {
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

void connection_list(const commands::Arguments& args,
                     const SessionManager& sessions,
                     log::Logger& logger) {
	if(!sessions.count()) {
		LOG_CONSOLE(logger, "No connected sessions to display");
		return;
	}

	std::stringstream stream;
	bprinter::TablePrinter table(&stream);
	table.AddColumn("ID", 5);
	table.AddColumn("Remote", 22);
	table.AddColumn("Identity", 25);
	table.AddColumn("State", 25);
	table.PrintHeader();

	std::size_t connections = 0; // count could change outside of iteration

	for(const auto& session : sessions) {
		const auto& [id, client] = session;
		table << id;
		table << client->connection().remote_address();
		table << client->handler().client_identify();
		table << ClientState_to_string(client->handler().state());
		++connections;
	}

	table.PrintFooter();
	LOG_CONSOLE(logger, "Displaying {} connections\n{}", connections, stream.str());
}

void print_connection_stats_header(bprinter::TablePrinter& table) {
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
}

void print_connection_stats_footer(bprinter::TablePrinter& table) {
	table.PrintFooter();
}

void print_connection_stats(bprinter::TablePrinter& table, SessionManager::SessionID id, const Client* client) {
	const auto stats = client->connection().stats();
	table << id;
	table << client->connection().remote_address();
	table << stats.latency;
	table << stats.messages_in;
	table << stats.messages_out;
	table << stats.bytes_in;
	table << stats.bytes_out;
	table << stats.async_receives;
	table << stats.async_sends;
}

void connection_statistics(const commands::Arguments& args,
						   const EventDispatcher& dispatcher,
                           const SessionManager& sessions,
                           log::Logger& logger) {
	if(!sessions.count()) {
		LOG_CONSOLE(logger, "No connected sessions to display");
		return;
	}

	std::size_t connections = 0;

	if(args.contains("id")) {
		const auto id = args["id"].as<SessionManager::SessionID>();
		const auto ident = sessions.client_ident(id);
		auto client = sessions.client(id);

		if(!ident || !client) {
			LOG_CONERR(logger, "Could not locate connection {}", id);
			return;
		}

		// it's only safe to interact with a client from within a session
		// iteration or by dispatching tasks/events to it - never interact
		// with a client outside of these two mechanisms
		dispatcher.exec(*ident, [&, id, client] {
			std::stringstream stream;
			bprinter::TablePrinter table(&stream);
			print_connection_stats_header(table);
			print_connection_stats(table, id, client);
			print_connection_stats_footer(table);
			LOG_CONSOLE(logger, "Displaying statistics for connection {}\n{}", id, stream.str());
		});
	} else {
		std::stringstream stream;
		bprinter::TablePrinter table(&stream);
		print_connection_stats_header(table);

		for(const auto& session : sessions) {
			const auto& [k, v] = session;
			print_connection_stats(table, k, v.get());
			++connections;
		}

		print_connection_stats_footer(table);
		LOG_CONSOLE(logger, "Displaying statistics for {} connections\n{}", connections, stream.str());
	}
}

void display_ident(const SessionManager& sessions, log::Logger& logger, SessionManager::SessionID id) {
	auto ident = sessions.client_ident(id);

	if(ident) {
		LOG_CONSOLE(logger, "Connection {} is RPC reference {}", id, ident->to_string());
	} else {
		LOG_CONERR(logger, "Unable to find connection {}", id);
	}
}

commands::ScopedCommand add_connections_commands(commands::Command& registry,
                                                 utility::CommandExecutor& exec,
												 const EventDispatcher& dispatcher,
												 const SessionManager& sessions,
                                                 log::Logger& logger) {
	auto root = commands::create("connections")
		->description("Commands for connection & session management");

	root->insert("list")
		->description("Display connections overview")
		->handler(exec([&](const commands::Arguments& args) {
			connection_list(args, sessions, logger);
		}));

	root->insert("kick")
		->description("Kick a specified connection")
		->argument<SessionManager::SessionID>("id")
		->handler(exec([&](const commands::Arguments& args) {
			kick_connection(
				args["id"].as<SessionManager::SessionID>(), sessions, dispatcher, logger
			);
		}));

	root->insert("netstat")
		->description("Display network stats for specified or all connections")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			connection_statistics(args, dispatcher, sessions, logger);
		}));

	root->insert("log")
		->description("Enable or disable connection packet logging")
		->argument<SessionManager::SessionID>("id")
		->argument<bool>("bool")
		->handler(exec([&](const commands::Arguments& args) {
			toggle_logging(
				args["id"].as<SessionManager::SessionID>(), 
				args["bool"].as<bool>(), sessions, dispatcher, logger
			);
		}));

	root->insert("notify")
		->description("Send a system notification to all or a specific connection")
		->argument<std::string>("message")
		->argument<SessionManager::SessionID>("id", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			system_message(args, sessions, dispatcher, logger, SystemMessage::Type::console);
		}));

	root->insert("message")
		->description("Send a system message to all or a specific connection")
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

	root->insert("ident")
		->description("Display a connection's RPC identity")
		->argument<SessionManager::SessionID>("id")
		->handler(exec([&](const commands::Arguments& args) {
			display_ident(sessions, logger, args["id"].as<SessionManager::SessionID>());
		}));

	// register scoped root 'connections' command
	return registry.scoped_insert(root);
}

} // realm, ember