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
#include <boost/asio/strand.hpp>
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
	using OnError = std::function<void(std::string_view)>;

	boost::asio::io_context::strand strand_;
	std::shared_ptr<std::atomic_bool> stop_flag_;
	std::shared_ptr<std::recursive_mutex> stop_lock_;
	OnError error_;

public:
	CommandExecutor(boost::asio::io_context& io_context, OnError&& error_cb)
		: strand_(io_context)
		, stop_flag_(std::make_shared<std::atomic_bool>(false))
		, stop_lock_(std::make_shared<std::recursive_mutex>())
		, error_(std::move(error_cb)) { }

	~CommandExecutor() {
		signal_stop();
	}

	void signal_stop() {
		std::lock_guard guard(*stop_lock_);
		*stop_flag_ = true;
	}

	template<typename Handler>
	auto operator()(Handler&& handler) {
		return wrap(handler);
	}

	template<typename Handler>
	auto wrap(Handler&& handler) {
		return [&, lock = stop_lock_, handler = std::forward<Handler>(handler)](auto&&... args) {
			// ensure the executor cannot be stopped or destroyed before we can post the real handler
			std::lock_guard guard(*lock);

			if(stop_flag_->load()) {
				error_("command handler is shutting down");
				return;
			}

			boost::asio::post(strand_, [error = error_, stop = stop_flag_, handler = std::move(handler), args...]() {
				// we need to check again, state may have changed between post and execution
				if(stop->load()) {
					error("command handler is shutting down");
					return;
				}

				try {
					handler(args...);
				} catch(const std::exception& e) {
					auto reason = std::format(R"(encountered exception, '{}')", e.what());
					error(reason);
				} catch(...) {
					auto reason = std::format(R"(encountered exception")");
					error(reason);
				}
			});
		};
	}
};

} // utility, ember