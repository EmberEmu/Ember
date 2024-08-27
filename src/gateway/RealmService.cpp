/*
 * Copyright (c) 2015 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmService.h"
#include <logger/Logging.h>
#include <utility>

namespace em = ember::messaging;

namespace ember {

RealmService::RealmService(Realm realm, spark::Service& spark, spark::ServiceDiscovery& discovery,
                           log::Logger* logger)
                           : realm_(realm), spark_(spark), discovery_(discovery), logger_(logger) {
	REGISTER(em::realm::Opcode::CMSG_REALM_STATUS, em::realm::RequestRealmStatus, RealmService::send_status);

	spark_.dispatcher()->register_handler(this, em::Service::GATEWAY, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::GATEWAY);
}

RealmService::~RealmService() {
	discovery_.remove_service(em::Service::GATEWAY);
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::on_message(const spark::Link& link, const spark::Message& message) {
	const auto handler = handlers_.find(static_cast<messaging::realm::Opcode>(message.opcode));

	if(handler == handlers_.end()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Unhandled message received from "
			<< link.description << LOG_ASYNC;
		return;
	}

	if(!handler->second.verify(message)) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Bad message received from "
			<< link.description << LOG_ASYNC;
		return;
	}

	handler->second.handle(link, message);
}

void RealmService::set_online() {
	realm_.flags &= ~Realm::Flags::OFFLINE;
	broadcast_status();
}

void RealmService::set_offline() {
	realm_.flags |= Realm::Flags::OFFLINE;
	broadcast_status();
}

void RealmService::send_status(const spark::Link& link, const spark::Message& message) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	constexpr auto opcode = std::to_underlying(em::realm::Opcode::SMSG_REALM_STATUS);
	auto fbb = build_status();
	spark_.send(link, opcode, fbb, message.token);
}

std::shared_ptr<flatbuffers::FlatBufferBuilder> RealmService::build_status() const {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	auto fb_name = fbb->CreateString(realm_.name);
	auto fb_ip = fbb->CreateString(realm_.ip);

	em::realm::RealmStatusBuilder builder(*fbb);
	builder.add_id(realm_.id);
	builder.add_name(fb_name);
	builder.add_ip(fb_ip);
	builder.add_flags(std::to_underlying(realm_.flags));
	builder.add_population(realm_.population);
	builder.add_category(std::to_underlying(realm_.category));
	builder.add_region(std::to_underlying(realm_.region));
	builder.add_type(std::to_underlying(realm_.type));
	fbb->Finish(builder.Finish());

	return fbb;
}

void RealmService::broadcast_status() const {
	auto fbb = build_status();
	spark_.broadcast(em::Service::GATEWAY, spark::ServicesMap::Mode::CLIENT, fbb);
}

void RealmService::on_link_up(const spark::Link& link) { 
	LOG_DEBUG(logger_) << "Link up: " << link.description << LOG_ASYNC;
}

void RealmService::on_link_down(const spark::Link& link) {
	LOG_DEBUG(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

} // ember