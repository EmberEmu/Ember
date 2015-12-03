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

RealmService::RealmService(Realm realm, spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger)
                           : realm_(realm), spark_(spark), discovery_(discovery), logger_(logger) { 
	spark_.dispatcher()->register_handler(this, em::Service::RealmStatus, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::RealmStatus);
}

RealmService::~RealmService() {
	discovery_.remove_service(em::Service::RealmStatus);
	spark_.dispatcher()->remove_handler(this);
}

void RealmService::handle_message(const spark::Link& link, const em::MessageRoot* root) {
	switch(root->data_type()) {
		case em::Data::RequestRealmStatus:
			send_realm_status(link, root);
			break;
	}
}

void RealmService::set_realm_online() {
	realm_.flags = static_cast<Realm::Flag>(realm_.flags & ~Realm::Flag::OFFLINE);
	broadcast_realm_status();
}

void RealmService::set_realm_offline() {
	realm_.flags = static_cast<Realm::Flag>(realm_.flags | Realm::Flag::OFFLINE);
	broadcast_realm_status();
}

void RealmService::send_realm_status(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	auto fbb = build_realm_status(root);
	spark_.send(link, std::move(fbb));
}

std::unique_ptr<flatbuffers::FlatBufferBuilder> RealmService::build_realm_status(const em::MessageRoot* root) {
	auto fbb = std::make_unique<flatbuffers::FlatBufferBuilder>();
	em::realm::RealmStatusBuilder rsb(*fbb);
	rsb.add_id(realm_.id);
	rsb.add_name(fbb->CreateString(realm_.name));
	rsb.add_ip(fbb->CreateString(realm_.ip));
	rsb.add_flags(realm_.flags);
	rsb.add_population(realm_.population);
	rsb.add_timezone(realm_.timezone);
	rsb.add_type(realm_.type);
	auto data_offset = rsb.Finish();


	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::RealmStatus);
	mrb.add_data_type(em::Data::RealmStatus);
	mrb.add_data(data_offset.Union());

	if(root) {
		spark_.set_tracking_data(root, mrb, fbb.get());
	}

	auto mloc = mrb.Finish();
	fbb->Finish(mloc);

	return fbb;
}

void RealmService::broadcast_realm_status() {
	auto fbb = build_realm_status();
	spark_.broadcast(em::Service::RealmStatus, spark::ServicesMap::Mode::CLIENT, std::move(fbb));
}

void RealmService::handle_link_event(const spark::Link& link, spark::LinkState event) {
	switch(event) {
		case spark::LinkState::LINK_UP:
			LOG_DEBUG(logger_) << "Link up: " << link.description << LOG_ASYNC;
			break;
		case spark::LinkState::LINK_DOWN:
			LOG_DEBUG(logger_) << "Link down: " << link.description << LOG_ASYNC;
			break;
	}
}

} // ember