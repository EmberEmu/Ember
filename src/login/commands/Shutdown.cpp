/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Shutdown.h"
#include <shared/utility/Utility.h>
#include <atomic>
#include <memory>

using namespace std::chrono_literals;

namespace ember {

void handle_shutdown_command(const commands::Arguments& arguments,
                             boost::asio::steady_timer& timer,
							 std::shared_ptr<std::atomic_bool> flag,
                             shutdown::OnInitiate on_initiate,
                             shutdown::OnExpire on_expire) {
	*flag = true;

	if(arguments.empty()) {
		timer.expires_after(0s);
	} else {
		auto time = std::chrono::seconds(arguments["seconds"].as<std::uint32_t>());
		timer.expires_after(time);
		on_initiate(time);
	}

	timer.async_wait([flag, on_expire = std::move(on_expire)](const auto& ec) {
		if(ec) {
			return;
		}

		bool expected = true;

		if(!flag->compare_exchange_strong(expected, false)) {
			return;
		}

		on_expire();
	});
}

void handle_remaining_command(boost::asio::steady_timer& timer,
                              std::shared_ptr<std::atomic_bool> flag,
                              shutdown::OnRemaining on_remaining) {
	const auto time = std::chrono::duration_cast<std::chrono::seconds>(
		timer.expiry() - std::chrono::steady_clock::now()
	);

	auto state = *flag? shutdown::State::pending : shutdown::State::not_pending;
	on_remaining(state, time);
}

void handle_cancel_command(boost::asio::steady_timer& timer,
                           std::shared_ptr<std::atomic_bool> flag,
                           shutdown::OnCancel on_cancel) {
	bool expected = true;

	if(flag->compare_exchange_strong(expected, false)) {
		timer.cancel();
		on_cancel(shutdown::State::pending);
	} else {
		on_cancel(shutdown::not_pending);
	}
}

void register_shutdown_command(commands::Registry& registry,
                               boost::asio::steady_timer& timer,
                               shutdown::OnInitiate on_initiate,
                               shutdown::OnCancel on_cancel,
                               shutdown::OnExpire on_expire,
                               shutdown::OnRemaining on_remaining) {
	auto flag = std::make_shared<std::atomic_bool>(false);

	auto root = registry.insert("shutdown")
		->description("Shuts the service down")
		->argument<std::uint32_t>("seconds")
		->handler([&, flag, on_initiate, on_expire](auto arguments) {
			handle_shutdown_command(arguments, timer, flag, on_initiate, on_expire);
		});

	root->insert("cancel") // subcommand
		->description("Cancels pending shutdown")
		->handler([&, flag, on_cancel](auto arguments) {
			handle_cancel_command(timer, flag, on_cancel);
		});

	root->insert("remaining")
		->description("View the time remaining until shutdown")
		->handler([&, flag, on_remaining](auto arguments) {
			handle_remaining_command(timer, flag, on_remaining);
		});
}

} // ember