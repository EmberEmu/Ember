/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "HelloService.h"
#include <logger/Logger.h>

namespace ember {

HelloService::HelloService(spark::v2::Server& server)
	: services::HelloService(server) {}

void HelloService::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_GLOB << "Server: Link up" << LOG_SYNC;
}

void HelloService::on_link_down(const spark::v2::Link& link) {

}

void HelloService::handle_say_hello(const messaging::Hello::HelloRequest* msg,
                                    messaging::Hello::HelloReplyT& reply) {
	LOG_INFO_GLOB << "[HelloService] Received message: " << msg->name()->c_str() << LOG_SYNC;
	reply.message = "Greetings, this is the reply from HelloService!";
}

}