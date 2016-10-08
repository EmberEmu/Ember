/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmService.h"
#include "RealmList.h"

namespace em = ember::messaging;

namespace ember {

RealmService::RealmService(RealmList& realms, spark::Service& spark,
                           spark::ServiceDiscovery& s_disc, log::Logger* logger)
                           : realms_(realms), spark_(spark), s_disc_(s_disc), logger_(logger) {
	spark_.dispatcher()->register_handler(
		this, em::Service::RealmStatus,
		spark::EventDispatcher::Mode::CLIENT
	);

	listener_ = std::move(s_disc_.listener(em::Service::RealmStatus,
	                      std::bind(&RealmService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

RealmService::~RealmService() {
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::on_message(const spark::Link& link, const ResponseToken& token, const em::MessageRoot* root) {
	switch(root->data_type()) {
		case em::Data::RealmStatus:
			handle_realm_status(link, root);
			break;
		default:
			LOG_DEBUG(logger_) << "Unhandled realm status message from " << link.description << LOG_ASYNC;
	}
}

void RealmService::handle_realm_status(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::realm::RealmStatus*>(root->data());

	if(!msg->name() || !msg->id() || !msg->ip()) {
		LOG_WARN(logger_) << "Incompatible realm status update from " << link.description << LOG_ASYNC;
	}

	// update everything rather than bothering to only set changed fields
	Realm realm;
	realm.id = msg->id();
	realm.ip = msg->ip()->str();
	realm.name = msg->name()->str();
	realm.population = msg->population();
	realm.type = static_cast<Realm::Type>(msg->type());
	realm.flags = static_cast<Realm::Flags>(msg->flags());
	realm.category = static_cast<dbc::Cfg_Categories::Category>(msg->category());
	realm.region = static_cast<dbc::Cfg_Categories::Region>(msg->region());
	realms_.add_realm(realm);

	LOG_INFO(logger_) << "Updated realm information for " << realm.name << LOG_ASYNC;

	// keep track of this link's realm ID so we can mark it as offline if it disappears
	known_realms_[link.uuid] = msg->id();
}

void RealmService::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link to realm gateway established" << LOG_ASYNC;
	request_realm_status(link);
}

void RealmService::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link to realm gateway closed" << LOG_ASYNC;
	mark_realm_offline(link);
}

void RealmService::service_located(const messaging::multicast::LocateAnswer* message) {
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

	Realm realm = realms_.get_realm(it->second);
	realm.flags = realm.flags | Realm::Flags::OFFLINE;
	realms_.add_realm(realm);

	LOG_INFO(logger_) << "Set gateway for " << realm.name <<  " to offline" << LOG_ASYNC;
}

void RealmService::request_realm_status(const spark::Link& link) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::RealmStatus, 0, 0,
	                                         em::Data::RequestRealmStatus, 0);
	fbb->Finish(msg);

	if(spark_.send(link, fbb) != spark::Service::Result::OK) {
		LOG_DEBUG(logger_) << "Realm status request failed, " << link.description << LOG_ASYNC;
	}
}

} // ember