/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Command.h>
#include <shared/utility/shutdown/Shutdown.h>
#include <shared/utility/Utility.h>
#include <boost/asio/io_context.hpp>
#include <format>
#include <functional>
#include <csignal>

namespace ember::shutdown {

using Log = std::function<void(std::string_view)>;

inline void install(commands::Command& registry, boost::asio::io_context& ioc, Log log) {
	shutdown::Handlers handlers {
		.on_initiate = [log](auto time) {
			log(std::format("Server will shut down in {}", utility::time_duration_format(time)));
		},
			
		.on_cancel = [log](auto state) {
			if(state == shutdown::State::pending) {
				log("Server shutdown has been cancelled");
			} else {
				log("No shutdown is pending");
			}
		},
			
		.on_expire = [log] {
			log("Server is shutting down now");
			std::raise(SIGINT);
		}, 
			
		.on_remaining = [log](auto state, auto time) {
			if(state == shutdown::State::pending) {
				log(std::format("Server will shut down in {}", utility::time_duration_format(time)));
			} else {
				log("No shutdown is pending");
			}
		}
	};

	shutdown::register_command(registry, ioc, std::move(handlers));
}

} // shutdown, ember