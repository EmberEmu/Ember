/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include <spark/temp/MessageRoot_generated.h>

namespace ember {

Service::Service(spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger)
                 : spark_(spark), discovery_(discovery), logger_(logger) {
	spark_.dispatcher()->register_handler(this, messaging::Service::Account, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(messaging::Service::Account);
}

Service::~Service() {
	discovery_.remove_service(messaging::Service::Account);
	spark_.dispatcher()->remove_handler(this);
}

void Service::handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {
	switch(msg->data_type()) {
		case messaging::Data::RegisterSession:
			register_session(link, static_cast<const messaging::RegisterSession*>(msg->data()));
			break;
		case messaging::Data::LocateSession:
			locate_session(link, static_cast<const messaging::LocateSession*>(msg->data()));
			break;
		default:
			LOG_DEBUG(logger_) << "Service received unhandled message type" << LOG_ASYNC;
	}
}

void Service::register_session(const spark::Link& link, const messaging::RegisterSession* msg) {

}

void Service::locate_session(const spark::Link& link, const messaging::LocateSession* msg) {

}

void Service::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_DEBUG(logger_) << "Link" << LOG_ASYNC;
}

} //ember