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

Service::Service(spark::Service& spark, log::Logger* logger) : spark_(spark), logger_(logger) { 
	spark_.dispatcher()->register_handler(this, messaging::Service::Service_Login,
	                                      spark::EventDispatcher::Mode::SERVER);
}

Service::~Service() {
	spark_.dispatcher()->remove_handler(this);
}

void Service::handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {
	LOG_DEBUG(logger_) << "Message" << LOG_ASYNC;
}

void Service::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_DEBUG(logger_) << "Link" << LOG_ASYNC;
}


} //ember