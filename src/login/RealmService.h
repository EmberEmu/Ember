/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Service.h>
#include <spark/ServiceDiscovery.h>
#include <logger/Logging.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/functional/hash.hpp>
#include <functional>
#include <memory>
#include <unordered_map>

namespace ember {

class RealmList;

class RealmService final : public spark::EventHandler {
	RealmList& realms_;
	spark::Service& spark_;
	spark::ServiceDiscovery& s_disc_;
	log::Logger* logger_;
	std::unique_ptr<spark::ServiceListener> listener_;
	mutable boost::uuids::random_generator generate_uuid; // functor
	std::unordered_map<boost::uuids::uuid, std::uint32_t, boost::hash<boost::uuids::uuid>> known_realms_;
	spark::Link link_;

	void service_located(const messaging::multicast::LocateAnswer* message);
	void request_realm_status(const spark::Link& link);
	void mark_realm_offline(const spark::Link& link);
	void handle_realm_status(const spark::Link& link, const messaging::MessageRoot* root);

public:
	RealmService(RealmList& realms, spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger);
	~RealmService();

	void on_message(const spark::Link& link, const spark::ResponseToken& token, const void* root /*temp*/) override;
	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;
};

} // ember