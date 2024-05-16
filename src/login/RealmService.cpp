/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmService.h"
#include "RealmList.h"
#include <utility>
#include <cassert>

namespace em = ember::messaging;

namespace ember {

RealmService::RealmService(RealmList& realms, spark::Service& spark,
                           spark::ServiceDiscovery& s_disc, log::Logger* logger)
                           : realms_(realms), spark_(spark), s_disc_(s_disc), logger_(logger) {
	REGISTER(em::realm::Opcode::SMSG_REALM_STATUS, em::realm::RealmStatus, RealmService::handle_realm_status);

	spark_.dispatcher()->register_handler(
		this, em::Service::GATEWAY,
		spark::EventDispatcher::Mode::CLIENT
	);

	listener_ = std::move(s_disc_.listener(em::Service::GATEWAY,
	                      std::bind(&RealmService::service_located, this, std::placeholders::_1)));
	listener_->search();

}

RealmService::~RealmService() {
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::on_message(const spark::Link& link, const spark::Message& message) {
	auto handler = handlers_.find(static_cast<messaging::realm::Opcode>(message.opcode));

	if(handler == handlers_.end()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "Unhandled realm message from "
			<< link.description << LOG_ASYNC;
		return;
	}

	if(!handler->second.verify(message)) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "Bad message received from "
			<< link.description << LOG_ASYNC;
		return;
	}

	handler->second.handle(link, message);
}

void RealmService::handle_realm_status(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto data = flatbuffers::GetRoot<messaging::realm::RealmStatus>(message.data);

	if(!data->name() || !data->id() || !data->ip()) {
		LOG_WARN(logger_) << "Incompatible realm status update from " << link.description << LOG_ASYNC;
		return;
	}

	// update everything rather than bothering to only set changed fields
	Realm realm;
	realm.id = data->id();
	realm.ip = data->ip()->str();
	realm.name = data->name()->str();
	realm.population = data->population();
	realm.type = static_cast<Realm::Type>(data->type());
	realm.flags = static_cast<Realm::Flags>(data->flags());
	realm.category = static_cast<dbc::Cfg_Categories::Category>(data->category());
	realm.region = static_cast<dbc::Cfg_Categories::Region>(data->region());
	realms_.add_realm(realm);

	LOG_INFO(logger_) << "Updated realm information for " << realm.name << LOG_ASYNC;

	// keep track of this link's realm ID so we can mark it as offline if it disappears
	known_realms_[link.uuid] = data->id();
}

void RealmService::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link to realm gateway established" << LOG_ASYNC;
	request_realm_status(link);
}

void RealmService::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link to realm gateway closed" << LOG_ASYNC;
	mark_realm_offline(link);
}

void RealmService::service_located(const messaging::multicast::LocateResponse* message) {
	LOG_DEBUG(logger_) << "Located realm gateway at " << message->ip()->str()
	                   << ":" << message->port() << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void RealmService::mark_realm_offline(const spark::Link& link) {
	auto it = known_realms_.find(link.uuid);

	// if we connected but didn't receive a valid status message, return
	if(it == known_realms_.end()) {
		return;
	}

	std::optional<Realm> realm = realms_.get_realm(it->second);
	assert(realm);
	realm->flags |= Realm::Flags::OFFLINE;
	realms_.add_realm(*realm);

	LOG_INFO(logger_) << "Set gateway for " << realm->name <<  " to offline" << LOG_ASYNC;
}

void RealmService::request_realm_status(const spark::Link& link) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	constexpr auto opcode = std::to_underlying(messaging::realm::Opcode::CMSG_REALM_STATUS);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	messaging::realm::RequestRealmStatusBuilder msg(*fbb);
	msg.Finish();

	if(spark_.send(link, opcode, fbb) != spark::Service::Result::OK) {
		LOG_DEBUG(logger_) << "Realm status request failed, " << link.description << LOG_ASYNC;
	}
}

} // ember