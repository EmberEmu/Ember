/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "RealmStatus_generated.h"
#include <shared/Realm.h>
#include <spark/Service.h>
#include <spark/Common.h>
#include <spark/ServiceDiscovery.h>
#include <spark/Helpers.h>
#include <logger/Logging.h>
#include <memory>

namespace ember {

class RealmService final : public spark::EventHandler {
	Realm realm_;
	spark::Service& spark_;
	spark::ServiceDiscovery& discovery_;
	log::Logger* logger_;
	std::unordered_map<messaging::realm::Opcode, spark::LocalDispatcher> handlers_;
	
	std::shared_ptr<flatbuffers::FlatBufferBuilder> build_status() const;
	void broadcast_status() const;
	void send_status(const spark::Link& link, const spark::Message& message) const;

public:
	RealmService(Realm realm, spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger);
	~RealmService();

	void on_message(const spark::Link& link, const spark::Message& message) override;
	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;

	void set_online();
	void set_offline();
};

} // ember