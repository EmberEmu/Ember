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

void register_shutdown_command(commands::PrefixedRegistry& cmd_register,
                               boost::asio::steady_timer& timer,
                               bool& pending_flag,
                               shutdown::OnInitiate on_initiate,
                               shutdown::OnCancel on_cancel,
                               shutdown::OnExpire on_expire) {
	auto root = cmd_register("shutdown")
		->description("Shuts the service down")
		->argument("seconds", commands::args::Type::at_uint32)
		->handler([&, on_initiate, on_expire](auto arguments) {
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
	});
	
	root->insert("cancel")
		->description("Cancels pending shutdown")
		->handler([&, on_cancel](auto arguments) {
			if(pending_flag) {
				timer.cancel();
				on_cancel();
			}

			pending_flag = false;
		}
	);

	pending_flag = false;
}

} // ember