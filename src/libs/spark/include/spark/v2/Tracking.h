/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Link.h>
#include <spark/v2/Common.h>
#include <logger/LoggerFwd.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <mutex>
#include <cstdint>

namespace ember::spark::v2 {

class Tracking final {
	struct Request {
		Request(boost::asio::io_context& service,
		        boost::uuids::uuid id,
		        Link link,
		        TrackedHandler handler)
		        : timer(service), id(id), handler(handler), link(std::move(link)) { }

		boost::asio::steady_timer timer;
		boost::uuids::uuid id;
		TrackedHandler handler;
		const Link link;
	};

	boost::unordered_flat_map<boost::uuids::uuid, std::unique_ptr<Request>,
	                          boost::hash<boost::uuids::uuid>> handlers_;

	boost::asio::io_context& io_context_;
	log::Logger* logger_;

	void request_timeout(const boost::uuids::uuid& id, Link link,
	                     const boost::system::error_code& ec);

public:
	Tracking(boost::asio::io_context& io_context, log::Logger* logger);
	~Tracking();

	void on_message(const Link& link, boost::uuids::uuid uuid, const Message& message);
	void shutdown();

	void register_tracked(const Link& link,
	                      boost::uuids::uuid id,
	                      TrackedHandler handler,
	                      std::chrono::milliseconds timeout);
};

} // spark, ember