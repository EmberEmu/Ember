/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SessionService.h"
#include <functional>

namespace ember {

SessionService::SessionService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger)
                               : spark_(spark), s_disc_(s_disc), logger_(logger) {
	spark_.dispatcher()->register_handler(this, messaging::Service::Service_Login,
	                                      spark::EventDispatcher::Mode::CLIENT);
	listener_ = std::move(s_disc_.listener(messaging::Service::Service_Login,
	                      std::bind(&SessionService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

SessionService::~SessionService() {
	spark_.dispatcher()->remove_handler(this);
}

void SessionService::handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {
	LOG_DEBUG(logger_) << "Message" << LOG_ASYNC;
}

void SessionService::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_DEBUG(logger_) << "Link" << LOG_ASYNC;
}

void SessionService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Found service" << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

} //ember