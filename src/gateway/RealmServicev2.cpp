/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RealmServicev2.h"

namespace ember {

RealmServicev2::RealmServicev2(spark::v2::Server& server, Realm realm, log::Logger& logger)
	: services::RealmStatusv2Service(server),
	  realm_(realm),
	  logger_(logger) { }

void RealmServicev2::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up: {}", link.peer_banner);

	std::lock_guard guard(mutex);
	links_.emplace_back(link);
}

void RealmServicev2::on_link_down(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link closed: {}", link.peer_banner);

	std::lock_guard guard(mutex);

	if(auto it = std::ranges::find(links_, link); it != links_.end()) {
		links_.erase(it);
	}
}

messaging::RealmStatusv2::RealmStatusT RealmServicev2::status() {
	return messaging::RealmStatusv2::RealmStatusT {
		.id = realm_.id,
		.name = realm_.name,
		.ip = realm_.ip,
		.port = realm_.port,
		.address = realm_.address,
		.population = realm_.population,
		.type = std::to_underlying(realm_.type),
		.flags = std::to_underlying(realm_.flags),
		.category = std::to_underlying(realm_.category),
		.region = std::to_underlying(realm_.region),
	};
}

std::optional<messaging::RealmStatusv2::RealmStatusT>
RealmServicev2::handle_status_request(
	const messaging::RealmStatusv2::RequestRealmStatus* msg,
	const spark::v2::Link& link,
	const spark::v2::Token& token) {	
	return status();
}

void RealmServicev2::set_online() {
	realm_.flags &= ~Realm::Flags::OFFLINE;
	broadcast_status();
}

void RealmServicev2::set_offline() {
	realm_.flags |= Realm::Flags::OFFLINE;
	broadcast_status();
}

void RealmServicev2::broadcast_status() {
	auto realm_info = status();

	std::lock_guard guard(mutex);

	for(auto& link : links_) {
		send(realm_info, link);
	}
}

} // ember