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
	log::Logger* logger_;

public:
	Service(spark::Service& spark, log::Logger* logger);
	~Service();

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) override;
	void handle_link_event(const spark::Link& link, spark::LinkState event) override;
};

} //ember