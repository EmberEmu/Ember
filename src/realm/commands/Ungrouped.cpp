/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Ungrouped.h"
#include "../ServiceContextImpl.h"
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

} // unnamed

void add_ungrouped_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();
	auto scoped = registry.scoped_insert(uptime(*impl->cmd_exec, impl->start_time, logger));
	impl->commands.emplace_back(std::move(scoped));
}

} // realm, ember