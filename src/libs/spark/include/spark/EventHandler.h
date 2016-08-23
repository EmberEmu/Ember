/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Link.h>

namespace ember { namespace spark {

class EventHandler {
public:
	virtual void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) = 0;
	virtual void handle_link_event(const spark::Link& link, spark::LinkState event) = 0;

	virtual ~EventHandler() = default;
};

}} // spark, ember