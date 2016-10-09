/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmService.h"
#include <shared/util/EnumHelper.h>

namespace em = ember::messaging;

namespace ember {

RealmService::RealmService(Realm realm, spark::Service& spark, spark::ServiceDiscovery& discovery,
                           log::Logger* logger)
                           : realm_(realm), spark_(spark), discovery_(discovery), logger_(logger) {
	REGISTER(em::realm::Opcode::CMSG_REALM_STATUS, em::realm::RequestRealmStatus, RealmService::send_realm_status);

	spark_.dispatcher()->register_handler(this, em::Service::GATEWAY, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::GATEWAY);
}

RealmService::~RealmService() {
	discovery_.remove_service(em::Service::GATEWAY);
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::on_message(const spark::Link& link, const spark::Message& message) {
	auto handler = handlers_.find(static_cast<messaging::core::Opcode>(message.opcode));

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

	handler->second.handle(message);
}

void RealmService::set_realm_online() {
	realm_.flags &= ~Realm::Flags::OFFLINE;
	broadcast_realm_status();
}

void RealmService::set_realm_offline() {
	realm_.flags |= Realm::Flags::OFFLINE;
	broadcast_realm_status();
}

void RealmService::send_realm_status(const spark::Link& link, const spark::Message& message) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	const auto opcode = util::enum_value(em::realm::Opcode::SMSG_REALM_STATUS);
	auto fbb = build_status();
	spark_.send(link, opcode, fbb, message.token);
}

std::shared_ptr<flatbuffers::FlatBufferBuilder> RealmService::build_status() const {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	em::realm::RealmStatusBuilder rsb(*fbb);
	rsb.add_id(realm_.id);
	rsb.add_name(fbb->CreateString(realm_.name));
	rsb.add_ip(fbb->CreateString(realm_.ip));
	rsb.add_flags(util::enum_value(realm_.flags));
	rsb.add_population(realm_.population);
	rsb.add_category(util::enum_value(realm_.category));
	rsb.add_region(util::enum_value(realm_.region));
	rsb.add_type(util::enum_value(realm_.type));
	fbb->Finish(rsb.Finish());

	return fbb;
}

void RealmService::broadcast_realm_status() const {
	auto fbb = build_status();
	spark_.broadcast(em::Service::GATEWAY, spark::ServicesMap::Mode::CLIENT, std::move(fbb));
}

void RealmService::on_link_up(const spark::Link& link) { 
	LOG_DEBUG(logger_) << "Link up: " << link.description << LOG_ASYNC;
}

void RealmService::on_link_down(const spark::Link& link) {
	LOG_DEBUG(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

} // ember