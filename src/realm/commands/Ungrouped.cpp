/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Ungrouped.h"
#include "../ServiceContextImpl.h"
#include <logger/Logger.h>
#include <shared/utility/Utility.h>

namespace ember::realm {

namespace {

auto uptime(utility::CommandExecutor& exec, std::chrono::steady_clock::time_point start, log::Logger& logger) 
-> std::shared_ptr<commands::Command> {
	return commands::create("uptime")
		->description("Display service uptime")
		->handler(exec([&, start](auto&) {
			const auto uptime = std::chrono::steady_clock::now() - start;
			LOG_CONSOLE(logger, "Server has been up for {}", utility::time_duration_format(uptime));
		}
	));
}

auto queue(utility::CommandExecutor& exec, const RealmQueue& realm_queue, log::Logger& logger) 
-> std::shared_ptr<commands::Command> {
	return commands::create("queue")
		->description("Display authentication queue size")
		->handler(exec([&](auto&) {
			LOG_CONSOLE(logger, "Current realm queue size is {}", realm_queue.size());
		}
	));
}

} // unnamed

void add_ungrouped_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();
	auto& realm_queue = *impl->queue;

	auto command = uptime(*impl->cmd_exec, impl->start_time, logger);
	impl->commands.emplace_back(registry.scoped_insert(std::move(command)));

	command = queue(*impl->cmd_exec, realm_queue, logger);
	impl->commands.emplace_back(registry.scoped_insert(std::move(command)));
}

} // realm, ember