/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Service.h>
#include <spark/Listener.h>

namespace ember { namespace spark {

Service::Service(boost::asio::io_service& service, const std::string& interface, std::uint16_t port,
                 log::Logger* logger, log::Filter filter)
                 : service_(service), logger_(logger), filter_(filter), signals_(service, SIGINT, SIGTERM),
                   listener_(service, interface, port, sessions_, logger, filter) {
	signals_.async_wait(std::bind(&Service::shutdown, this));
}

void Service::shutdown() {
	LOG_DEBUG_FILTER(logger_, filter_) << "Spark service shutting down..." << LOG_ASYNC;
	listener_.shutdown();
	sessions_.stop_all();
}

void Service::connect(const std::string& host, std::uint16_t port) {

}

Router* Service::router() {
	return &router_;
}

}}