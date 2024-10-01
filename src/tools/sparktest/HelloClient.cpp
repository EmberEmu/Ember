/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "HelloClient.h"
#include <iostream>
#include <logger/Logger.h>

namespace ember {

HelloClient::HelloClient(spark::v2::Server& spark)
	: services::HelloClient(spark),
	  spark_(spark) {
	connect("127.0.0.1", 8000);
}

void HelloClient::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_GLOB << "Client: Link up" << LOG_SYNC;

	messaging::Hello::HelloRequestT msg;
	msg.name = "Aloha from the HelloClient!";
	flatbuffers::FlatBufferBuilder fbb;
	messaging::Hello::EnvelopeT env;
	env.message.Set(msg);
	auto packed = messaging::Hello::Envelope::Pack(fbb, &env);
	fbb.Finish(packed);

	auto channel = link.net.lock();

	if(channel) {
		channel->send(std::move(fbb));
	}

}

void HelloClient::on_link_down(const spark::v2::Link& link) {

}

void HelloClient::handle_say_hello_response(const messaging::Hello::HelloReply* msg) {
	LOG_INFO_GLOB << "[HelloClient] Received response: " << msg->message()->c_str() << LOG_SYNC;
}

}