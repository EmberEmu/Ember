/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Tracking.h>
#include <shared/FilterTypes.h>
#include <spark/v2/Message.h>
#include <algorithm>
#include <memory>

namespace sc = std::chrono;

namespace ember::spark::v2 {

Tracking::Tracking(boost::asio::io_context& io_context, log::Logger* logger)
                   : io_context_(io_context), logger_(logger) { }

Tracking::~Tracking() {

}

void Tracking::on_message(const Link& link, boost::uuids::uuid uuid, const Message& message) try {
	auto request = std::move(handlers_.at(uuid));
	handlers_.erase(uuid);

	if(link != request->link) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Tracked message receipient != sender" << LOG_ASYNC;
		return;
	}

	request->handler(link, false);
} catch(const std::out_of_range) {
	LOG_DEBUG_FILTER(logger_, LF_SPARK)
		<< "[spark] Received invalid or expired tracked message" << LOG_ASYNC;
}

void Tracking::register_tracked(const Link& link,
                                boost::uuids::uuid id,
                                TrackedHandler handler,
                                sc::milliseconds timeout) {
	auto request = std::make_unique<Request>(io_context_, id, link, handler);
	request->timer.expires_from_now(timeout);

	request->timer.async_wait([this, id, link](const boost::system::error_code& ec) {
		request_timeout(id, link, ec);
	});

	handlers_[id] = std::move(request);
}

void Tracking::request_timeout(const boost::uuids::uuid& id, Link link,
                               const boost::system::error_code& ec) {
	if(ec) { // timer was cancelled
		return;
	}

	// inform the handler that no response was received and erase
	auto request = std::move(handlers_.at(id));
	handlers_.erase(id);

	request->handler(link, false);
}

void Tracking::shutdown() {
	for(auto& handler : handlers_) {
		handler.second->timer.cancel();
	}
}

} // spark, ember