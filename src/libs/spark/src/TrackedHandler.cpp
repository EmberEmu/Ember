/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/TrackedHandler.h>
#include <boost/optional.hpp>

namespace sc = std::chrono;

namespace ember { namespace spark {

TrackedHandler::TrackedHandler(boost::asio::io_service& service, log::Logger* logger, log::Filter filter)
                               : service_(service), logger_(logger), filter_(filter) { }

void TrackedHandler::handle_message(const Link& link, const messaging::MessageRoot* message) try {
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

void TrackedHandler::handle_event(const Link& link, LinkState state) {
	// we don't care about this
}

void TrackedHandler::register_tracked(const Link& link, boost::uuids::uuid id,
									  TrackingHandler handler, sc::milliseconds timeout) {
	auto request = std::make_unique<Request>(service_, id, link, handler);
	request->timer.async_wait(std::bind(&TrackedHandler::timeout, this, id, std::placeholders::_1));
	request->timer.expires_from_now(timeout);

	std::lock_guard<std::mutex> guard(lock_);
	handlers_[id] = std::move(request);
}

void TrackedHandler::timeout(boost::uuids::uuid id, const boost::system::error_code& ec) {
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

void TrackedHandler::shutdown() {
	std::lock_guard<std::mutex> guard(lock_);

	for(auto& handler : handlers_) {
		handler.second->timer.cancel();
	}
}

}} // spark, ember