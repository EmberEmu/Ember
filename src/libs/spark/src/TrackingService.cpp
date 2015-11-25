/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/TrackingService.h>
#include <boost/optional.hpp>
#include <algorithm>

namespace sc = std::chrono;

namespace ember { namespace spark {

TrackingService::TrackingService(boost::asio::io_service& service, log::Logger* logger, log::Filter filter)
                                 : service_(service), logger_(logger), filter_(filter) { }

void TrackingService::handle_message(const Link& link, const messaging::MessageRoot* message) try {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	auto recv_id = message->tracking_id();

	if(recv_id->size() != boost::uuids::uuid::static_size()) {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Received tracked message with invalid UUID length" << LOG_ASYNC;
	}

	boost::uuids::uuid uuid;
	std::copy(recv_id->begin(), recv_id->end(), uuid.begin());

	std::unique_lock<std::mutex> guard(lock_);
	auto handler = std::move(handlers_.at(uuid));
	handlers_.erase(uuid);
	guard.unlock();

	if(link != handler->link) {
		LOG_WARN_FILTER(logger_, filter_)
			<< "[spark] Tracked message receipient != sender" << LOG_ASYNC;
	}

	handler->handler(boost::optional<const messaging::MessageRoot*>(message));
} catch(std::out_of_range) {
	LOG_DEBUG_FILTER(logger_, filter_)
		<< "[spark] Received invalid or expired tracked message" << LOG_ASYNC;
}

void TrackingService::handle_link_event(const Link& link, LinkState state) {
	// we don't care about this
}

void TrackingService::register_tracked(const Link& link, boost::uuids::uuid id,
                                       TrackingHandler handler, sc::milliseconds timeout) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	auto request = std::make_unique<Request>(service_, id, link, handler);
	request->timer.expires_from_now(timeout);
	request->timer.async_wait(std::bind(&TrackingService::timeout, this, id, std::placeholders::_1));

	std::lock_guard<std::mutex> guard(lock_);
	handlers_[id] = std::move(request);
}

void TrackingService::timeout(boost::uuids::uuid id, const boost::system::error_code& ec) {
	if(ec) { // timer was cancelled
		return;
	}

	// inform the handler that no response was received and erase
	std::unique_lock<std::mutex> guard(lock_);
	auto handler = std::move(handlers_.at(id));
	handlers_.erase(id);
	guard.unlock();

	handler->handler(boost::optional<const messaging::MessageRoot*>());
}

void TrackingService::shutdown() {
	std::lock_guard<std::mutex> guard(lock_);

	for(auto& handler : handlers_) {
		handler.second->timer.cancel();
	}
}

}} // spark, ember