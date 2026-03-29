/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Shutdown.h"
#include <shared/utility/Utility.h>
#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include <memory>

using namespace std::chrono_literals;

namespace ember::shutdown {

void handle_shutdown_command(const commands::Arguments& arguments,
                             std::shared_ptr<boost::asio::steady_timer> timer,
							 std::shared_ptr<std::atomic_bool> flag,
                             OnInitiate on_initiate,
                             OnExpire on_expire) {
	const bool previous = flag->exchange(true);

	if(arguments.empty()) {
		timer->expires_after(0s);
	} else {
		const auto seconds = arguments["seconds"].as<std::chrono::seconds>();

		if(seconds < 0s) {
			*flag = previous;
			return;
		}

		timer->expires_after(seconds);
		on_initiate(seconds);
	}

	timer->async_wait([flag, on_expire = std::move(on_expire)](const auto& ec) {
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

void handle_remaining_command(std::shared_ptr<boost::asio::steady_timer> timer,
                              std::shared_ptr<std::atomic_bool> flag,
                              OnRemaining on_remaining) {
	const auto time = std::chrono::duration_cast<std::chrono::seconds>(
		timer->expiry() - std::chrono::steady_clock::now()
	);

	auto state = *flag? shutdown::State::pending : shutdown::State::not_pending;
	on_remaining(state, time);
}

void handle_cancel_command(std::shared_ptr<boost::asio::steady_timer> timer,
                           std::shared_ptr<std::atomic_bool> flag,
                           OnCancel on_cancel) {
	bool expected = true;

	if(flag->compare_exchange_strong(expected, false)) {
		timer->cancel();
		on_cancel(shutdown::State::pending);
	} else {
		on_cancel(shutdown::not_pending);
	}
}

void register_command(commands::Command& registry, boost::asio::io_context& ioc, Handlers handlers) {
	auto flag = std::make_shared<std::atomic_bool>(false);
	auto timer = std::make_shared<boost::asio::steady_timer>(ioc);

	auto root = registry.insert("shutdown")
		->description("Shuts the service down")
		->argument<std::chrono::seconds>("seconds", commands::optional)
		->handler([&, timer, flag, initiate = std::move(handlers.on_initiate),
			expire = std::move(handlers.on_expire)](auto arguments) {
			handle_shutdown_command(arguments, timer, flag, initiate, expire);
		});

	root->insert("cancel") // subcommand
		->description("Cancels pending shutdown")
		->handler([&, timer, flag, cancel = std::move(handlers.on_cancel)](auto arguments) {
			handle_cancel_command(timer, flag, cancel);
		});

	root->insert("remaining")
		->description("View the time remaining until shutdown")
		->handler([&, timer, flag, remaining = std::move(handlers.on_remaining)](auto arguments) {
			handle_remaining_command(timer, flag, remaining);
		});
}

} // shutdown, ember