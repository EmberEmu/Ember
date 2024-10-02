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

 auto HelloService::handle_say_hello(const messaging::Hello::HelloRequest* msg,
                                     const spark::v2::Link& link,
                                     const spark::v2::Token& token)
                                     -> std::optional<messaging::Hello::HelloReplyT> {
	LOG_INFO_GLOB << "[HelloService] Received message: " << msg->name()->c_str() << LOG_SYNC;

	return messaging::Hello::HelloReplyT {
		.message = "Greetings, this is the reply from HelloService!"
	};
}

}