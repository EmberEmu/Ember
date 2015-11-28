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
		case messaging::Data::RegisterKey:
			register_session(link, msg);
			break;
		case messaging::Data::KeyLookup:
			locate_session(link, msg);
			break;
		default:
			LOG_DEBUG(logger_) << "Service received unhandled message type" << LOG_ASYNC;
	}
}

void Service::register_session(const spark::Link& link, const messaging::MessageRoot* msg) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = fbb->CreateVector(msg->tracking_id()->data(), msg->tracking_id()->size());
	auto resp = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, uuid, 1,
		messaging::Data::Response, messaging::account::CreateResponse(*fbb, messaging::account::Status::OK).Union());
	fbb->Finish(resp);

	spark_.send(link, fbb);
}

void Service::locate_session(const spark::Link& link, const messaging::MessageRoot* msg) {
	/*auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, msg->, 1,
		em::Data::KeyLookup, em::account::CreateKeyLookup(*fbb, account_id).Union());
	fbb->Finish(msg);

	spark_.send(link, fbb);*/
}

void Service::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_DEBUG(logger_) << "Link" << LOG_ASYNC;
}

} //ember