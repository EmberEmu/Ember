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
#include <memory>

namespace ember {

class SessionService final : public spark::EventHandler {
	spark::Service& spark_;
	spark::ServiceDiscovery& s_disc_;
	log::Logger* logger_;
	std::unique_ptr<spark::ServiceListener> listener_;
	
	void service_located(const messaging::multicast::LocateAnswer* message);

public:
	SessionService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger);
	~SessionService();

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) override;
	void handle_link_event(const spark::Link& link, spark::LinkState event) override;
};

} //ember