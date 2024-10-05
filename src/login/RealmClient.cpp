/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmClient.h"

namespace ember {

RealmClient::RealmClient(spark::v2::Server& server, RealmList& realmlist, log::Logger& logger)
	: services::RealmClient(server),
	  realmlist_(realmlist),
	  logger_(logger) {
	connect("127.0.0.1", 8002); // temp
}

void RealmClient::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up: {}", link.peer_banner);
	request_realm_status(link);
}

void RealmClient::on_link_down(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link closed: {}", link.peer_banner);
	mark_realm_offline(link);
}

void RealmClient::request_realm_status(const spark::v2::Link& link) {
	em::RequestStatusT msg{};
	send(msg, link);
}

void RealmClient::mark_realm_offline(const spark::v2::Link& link) {
	auto it = realms_.find(link.peer_banner);

	// if we connected but didn't get its information, return
	if(it == realms_.end()) {
		return;
	}

	std::optional<Realm> realm = realmlist_.get_realm(it->second);
	assert(realm);
	realm->flags |= Realm::Flags::OFFLINE;
	realmlist_.add_realm(*realm);

	LOG_INFO_ASYNC(logger_, "Set realm {} to offline", realm->name);
}

void RealmClient::handle_get_status_response(
	const spark::v2::Link& link,
	const em::Status* msg) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(!msg->name() || !msg->id() || !msg->ip() || !msg->address()) {
		LOG_WARN_ASYNC(logger_, "Incompatible realm status update from {}", link.peer_banner);
		return;
	}

	// update everything rather than bothering to only set changed fields
	Realm realm {
		.id = msg->id(),
		.name = msg->name()->str(),
		.ip = msg->ip()->str(),
		.port = msg->port(),
		.address = msg->address()->str(),
		.population = msg->population(),
		.type = static_cast<Realm::Type>(msg->type()),
		.flags = static_cast<Realm::Flags>(msg->flags()),
		.category = static_cast<dbc::Cfg_Categories::Category>(msg->category()),
		.region = static_cast<dbc::Cfg_Categories::Region>(msg->region())
	};

	LOG_INFO_ASYNC(logger_, "Updated realm information for {} ({}:)", realm.name, realm.address);
	realmlist_.add_realm(std::move(realm));

	// keep track of this link's realm ID so we can mark it as offline if it disappears
	realms_[link.peer_banner] = msg->id();
}

} // ember