/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/Link.h>
#include <spark/EventHandler.h>
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace ember { namespace spark {

class TrackingService : public EventHandler {
	struct Request {
		Request(boost::asio::io_service& service, boost::uuids::uuid id, Link link, TrackingHandler handler)
		        : timer(service), id(id), handler(handler), link(std::move(link)) { }

		boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer;
		boost::uuids::uuid id;
		TrackingHandler handler;
		const Link link;
	};

	std::unordered_map<boost::uuids::uuid, std::unique_ptr<Request>,
	                   boost::hash<boost::uuids::uuid>> handlers_;

	boost::asio::io_service& service_;
	log::Logger* logger_;
	std::mutex lock_;

	void timeout(boost::uuids::uuid id, Link link, const boost::system::error_code& ec);

public:
	TrackingService(boost::asio::io_service& service, log::Logger* logger);

	void handle_message(const Link& link, const messaging::MessageRoot* message);
	void handle_link_event(const Link& link, LinkState state);
	void register_tracked(const Link& link, boost::uuids::uuid id, TrackingHandler handler,
	                      std::chrono::milliseconds timeout);
	void shutdown();
};

}}