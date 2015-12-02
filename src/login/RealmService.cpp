/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmService.h"
#include <boost/uuid/uuid.hpp>

namespace em = ember::messaging;

namespace ember {

RealmService::RealmService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger)
                               : spark_(spark), s_disc_(s_disc), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::RealmStatus, spark::EventDispatcher::Mode::CLIENT);
	listener_ = std::move(s_disc_.listener(messaging::Service::RealmStatus,
	                      std::bind(&RealmService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

RealmService::~RealmService() {
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::handle_message(const spark::Link& link, const em::MessageRoot* root) {

}

void RealmService::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(event) {
		case spark::LinkState::LINK_UP:
			LOG_INFO(logger_) << "Link to realm gateway established" << LOG_ASYNC;
			link_ = link;
			break;
		case spark::LinkState::LINK_DOWN:
			LOG_INFO(logger_) << "Link to realm gateway closed" << LOG_ASYNC;
			break;
	}
}

void RealmService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Located realm gateway at " << message->ip()->str() << LOG_ASYNC; // todo
	spark_.connect(message->ip()->str(), message->port());
}

} // ember