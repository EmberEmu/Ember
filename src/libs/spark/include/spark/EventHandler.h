/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace spark {

struct Message;
struct Link;

class EventHandler {
public:
	virtual void on_message(const Link& link, const Message& message) = 0;
	virtual void on_link_up(const Link& link) = 0;
	virtual void on_link_down(const Link& link) = 0;

	virtual ~EventHandler() = default;
};

}} // spark, ember