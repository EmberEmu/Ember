/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <atomic>
#include <functional>
#include <utility>

namespace ember::utility {

class CommandExecutor {
	using OnFailure = std::function<void(void)>;

	boost::asio::io_context& ctx_;
	const std::atomic_bool& stop_flag_;
	OnFailure failure_cb_;

public:
	CommandExecutor(boost::asio::io_context& ctx, const std::atomic_bool& stop_flag, OnFailure&& failure_cb)
		: ctx_(ctx),
		  stop_flag_(stop_flag),
		  failure_cb_(failure_cb) { }

	template<typename Handler>
	auto operator()(Handler&& handler) {
		return [&, handler = std::forward<Handler>(handler)](auto&&... args) {
			boost::asio::post(ctx_, [&, handler = std::move(handler), args...]() {
				if(stop_flag_) {
					failure_cb_();
					return;
				}

				handler(args...);
			});
		};
	}
};

} // commands, ember