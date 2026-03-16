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
#include <string_view>
#include <utility>

namespace ember::utility {

/*
 * This class provides a wrapper for command handlers that ensures
 * their execution goes through the provided Asio io_context.
 * 
 * The purpose of this is to ensure that the service cannot be shut down
 * while there are still commands in flight, which could occur if they
 * were being run on a thread other than the one managing service lifetime.
 * 
 * The stop flag ensures that no further commands are dispatched once
 * a shutdown has been initiated, meaning in flight commands are
 * allowed to finish but no new commands may begin.
 */
class CommandExecutor {
	using OnFailure = std::function<void(std::string_view)>;

	boost::asio::io_context::strand& strand_;
	const std::atomic_bool& stop_flag_;
	OnFailure failure_cb_;

public:
	CommandExecutor(boost::asio::io_context::strand& strand, const std::atomic_bool& stop_flag, OnFailure&& failure_cb)
		: strand_(strand)
		, stop_flag_(stop_flag)
		, failure_cb_(std::move(failure_cb)) { }

	template<typename Handler>
	auto operator()(Handler&& handler) {
		return wrap(handler);
	}

	template<typename Handler>
	auto wrap(Handler&& handler) {
		return [&, handler = std::forward<Handler>(handler)](auto&&... args) {
			boost::asio::post(strand_, [&, handler = std::move(handler), args...]() {
				if(stop_flag_) {
					failure_cb_("command handler is shutting down");
					return;
				}

				try {
					handler(args...);
				} catch(const std::exception& e) {
					auto reason = std::format(R"(encountered exception, '{}')", e.what());
					failure_cb_(reason);
				} catch(...) {
					auto reason = std::format(R"(encountered exception")");
					failure_cb_(reason);
				}
			});
		};
	}
};

} // utility, ember