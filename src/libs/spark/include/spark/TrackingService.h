/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/Link.h>
#include <spark/EventHandler.h>
#include <logger/LoggerFwd.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace ember::spark::inline v1 {

class TrackingService final : public EventHandler {
	struct Request {
		Request(boost::asio::io_context& service, boost::uuids::uuid id, Link link,
		         TrackingHandler handler)
		        : timer(service), id(id), handler(handler), link(std::move(link)) { }

		boost::asio::steady_timer timer;
		boost::uuids::uuid id;
		TrackingHandler handler;
		const Link link;
	};

	std::unordered_map<boost::uuids::uuid, std::unique_ptr<Request>,
	                   boost::hash<boost::uuids::uuid>> handlers_;

	boost::asio::io_context& io_context_;
	log::Logger* logger_;
	std::mutex lock_;

	void timeout(const boost::uuids::uuid& id, Link link, const boost::system::error_code& ec);

public:
	TrackingService(boost::asio::io_context& io_context, log::Logger* logger);

	void on_message(const Link& link, const Message& message) override;
	void on_link_up(const Link& link) override;
	void on_link_down(const Link& link) override;

	void register_tracked(const Link& link, boost::uuids::uuid id, TrackingHandler handler,
	                      std::chrono::milliseconds timeout);
	void shutdown();
};

} // spark, ember