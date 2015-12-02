/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmService.h"

namespace em = ember::messaging;

namespace ember {

RealmService::RealmService(spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger)
                           : spark_(spark), discovery_(discovery), logger_(logger) { 
	spark_.dispatcher()->register_handler(this, em::Service::RealmStatus, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::RealmStatus);
}

RealmService::~RealmService() {
	discovery_.remove_service(em::Service::RealmStatus);
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::handle_message(const spark::Link& link, const em::MessageRoot* msg) {

}

void RealmService::handle_link_event(const spark::Link& link, spark::LinkState event) {

}

} // ember