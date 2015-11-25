/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Spark.h>
#include <spark/EventHandler.h>

namespace ember {

class Listener final : public spark::EventHandler {
	spark::Service& service_;

public:
	Listener(spark::Service& service);
	~Listener();

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg);
	void handle_link_event(const spark::Link& link, spark::LinkState event);
};

} // ember