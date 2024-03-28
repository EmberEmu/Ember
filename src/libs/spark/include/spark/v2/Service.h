/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Link.h>
#include <spark/v2/Message.h>
#include <string>

namespace ember::spark::v2 {

class Service {
public:
	virtual std::string service_type() = 0;
	virtual void on_message(const spark::v2::Link& link, const spark::v2::Message& message) = 0;
	virtual void on_link_up(const spark::v2::Link& link) = 0;
	virtual void on_link_down(const spark::v2::Link& link) = 0;

	virtual ~Service() = default;
};

} // spark, ember