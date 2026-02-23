/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Shutdown.h"
#include <shared/utility/Utility.h>

using namespace std::chrono_literals;

namespace ember {

void handle_shutdown_command(commands::args::Map& arguments,
                             boost::asio::steady_timer& timer,
                             bool& pending_flag,
                             shutdown::OnInitiate on_initiate,
                             shutdown::OnExpire on_expire) {
	if(arguments.empty()) {
		timer.expires_after(0s);
	} else {
		auto time = std::chrono::seconds(std::get<std::uint32_t>(arguments["seconds"]));
		timer.expires_after(time);
		on_initiate(time);
	}

	timer.async_wait([&](const auto& ec) {			
		if(!ec) {
			on_expire();
		}
	});

	pending_flag = true;
}

void handle_cancel_command(boost::asio::steady_timer& timer,
                           bool& pending_flag,
                           shutdown::OnCancel on_cancel) {
	if(pending_flag) {
		timer.cancel();
		on_cancel();
	}

	pending_flag = false;
}

void register_shutdown_command(commands::PrefixedRegistry& cmd_register,
                               boost::asio::steady_timer& timer,
                               bool& pending_flag,
                               shutdown::OnInitiate on_initiate,
                               shutdown::OnCancel on_cancel,
                               shutdown::OnExpire on_expire) {
	cmd_register("shutdown")
		->description("Shuts the service down")
		->argument("seconds", commands::args::Type::at_uint32)
		->handler([&, on_initiate, on_expire](auto arguments) {
			handle_shutdown_command(arguments, timer, pending_flag, on_initiate, on_expire);
		})->insert("cancel") // subcommand
			->description("Cancels pending shutdown")
			->handler([&, on_cancel](auto arguments) {
				handle_cancel_command(timer, pending_flag, on_cancel);
			}
		);

	pending_flag = false;
}

} // ember