/*
 * Copyright (c) 2015 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/TrackingService.h>
#include <logger/Logger.h>
#include <shared/FilterTypes.h>
#include <algorithm>
#include <optional>

namespace sc = std::chrono;

namespace ember::spark::inline v1 {

TrackingService::TrackingService(boost::asio::io_context& io_context, log::Logger* logger)
                                 : io_context_(io_context), logger_(logger) { }

void TrackingService::on_message(const Link& link, const Message& message) try {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << log_func << LOG_ASYNC;

	std::unique_lock guard(lock_);
	auto handler = std::move(handlers_.at(message.token.uuid));
	handlers_.erase(message.token.uuid);
	guard.unlock();

	if(link != handler->link) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Tracked message receipient != sender" << LOG_ASYNC;
		return;
	}

	handler->handler(link, message);
} catch(const std::out_of_range) {
	LOG_DEBUG_FILTER(logger_, LF_SPARK)
		<< "[spark] Received invalid or expired tracked message" << LOG_ASYNC;
}

void TrackingService::register_tracked(const Link& link, boost::uuids::uuid id, TrackingHandler handler,
                                       sc::milliseconds timeout) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << log_func << LOG_ASYNC;

	auto request = std::make_unique<Request>(io_context_, id, link, handler);
	request->timer.expires_from_now(timeout);
	request->timer.async_wait(std::bind(&TrackingService::timeout, this, id, link, std::placeholders::_1));

	std::lock_guard guard(lock_);
	handlers_[id] = std::move(request);
}

void TrackingService::timeout(const boost::uuids::uuid& id, Link link, const boost::system::error_code& ec) {
	if(ec) { // timer was cancelled
		return;
	}

	// inform the handler that no response was received and erase
	std::unique_lock guard(lock_);
	auto handler = std::move(handlers_.at(id));
	handlers_.erase(id);
	guard.unlock();

	handler->handler(link, std::nullopt);
}

void TrackingService::shutdown() {
	std::lock_guard guard(lock_);

	for(auto& handler : handlers_) {
		handler.second->timer.cancel();
	}
}

void TrackingService::on_link_up(const Link& link) {
	// we don't care about this
}

void TrackingService::on_link_down(const Link& link) {
	// we don't care about this
}

} // spark, ember