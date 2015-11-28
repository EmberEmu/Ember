/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Service.h>
#include <logger/Logging.h>

namespace ember {

class Service : public spark::EventHandler {
	spark::Service& spark_;
	spark::ServiceDiscovery& discovery_;
	log::Logger* logger_;

	void register_session(const spark::Link& link, const messaging::MessageRoot* msg);
	void locate_session(const spark::Link& link, const messaging::MessageRoot* msg);

public:
	Service(spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger);
	~Service();

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) override;
	void handle_link_event(const spark::Link& link, spark::LinkState event) override;
};

} //ember