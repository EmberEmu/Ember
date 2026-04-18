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
#include <shared/utility/DurationString.h>
#include <shared/utility/Utility.h>

namespace ember::realm {

namespace {

void initiate_shutdown(const commands::Arguments& args, ShutdownAnnouncer& spa, log::Logger& logger) {
	const auto time = args["time"].as<utility::DurationString>();
	auto announce = false;

	if(args.contains("announce")) {
		announce = args["announce"].as<bool>();
	}

	spa.set_after(time.value, announce);
	LOG_CONSOLE(logger, "Shutdown sequence started");
}

void cancel_shutdown(ShutdownAnnouncer& spa, log::Logger& logger) {
	if(!spa.pending()) {
		LOG_CONSOLE(logger, "No shutdown is pending");
		return;
	}

	spa.stop();
	LOG_CONSOLE(logger, "Server shutdown has been cancelled");
}

void ttl_shutdown(const ShutdownAnnouncer& spa, log::Logger& logger) {
	if(!spa.pending()) {
		LOG_CONSOLE(logger, "No shutdown is pending");
		return;
	}

	const auto duration = spa.expires() - std::chrono::steady_clock::now();
	LOG_CONSOLE(logger, "Server will shut down in {}", utility::time_duration_format(duration));
}

} // unnamed

void add_shutdown_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();
	auto& exec = *impl->cmd_exec;
	auto& spa = *impl->shutdown_pa;

	auto root = commands::create("shutdown")
		->description("Schedule realm shutdown (%d%h%s)")
		->argument<utility::DurationString>("time")
		->argument<bool>("announce", commands::optional)
		->handler(exec([&](const auto& args) {
			initiate_shutdown(args, spa, logger);
		}
	));

	root->insert("cancel")
		->description("Cancels pending shutdown")
		->argument<bool>("kick", commands::optional)
		->handler(exec([&](const auto&) {
			cancel_shutdown(spa, logger);
		}));

	root->insert("remaining")
		->description("View the time remaining until shutdown")
		->handler(exec([&](const auto&) {
			ttl_shutdown(spa, logger);
		}));

	auto scoped = registry.scoped_insert(root);
	impl->commands.emplace_back(std::move(scoped));
}

} // realm, ember