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

namespace impl {

commands::ScopedCommand add_uptime(commands::Command& registry, utility::CommandExecutor& exec,
                                   std::chrono::steady_clock::time_point& time, log::Logger& logger) {
	return registry.scoped_insert(commands::create("uptime")
		->description("Display service uptime")
		->handler(exec([&, time](auto&) {
			const auto uptime = std::chrono::steady_clock::now() - time;
			LOG_CONSOLE(logger, "Server has been up for {}", utility::time_duration_format(uptime));
		})
	));
}

} // impl

void add_ungrouped_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();

	impl->commands.emplace_back(
		impl::add_uptime(registry, *impl->cmd_exec, impl->start_time, logger)
	);
}

} // realm, ember