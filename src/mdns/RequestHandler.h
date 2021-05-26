/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Service.h>
#include <logger/Logging.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember::dns {

class RequestHandler final : public spark::EventHandler {
	log::Logger* logger_;

public:
	explicit RequestHandler(log::Logger* logger);
	void shutdown();

	void on_message(const spark::Link& link, const spark::Message& message) override;
	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;
};

} // dns, ember