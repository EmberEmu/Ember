/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

//#include "RequestHandler_Generated.h"
#include <spark/Service.h>
#include <logger/LoggerFwd.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember::dns {

class RequestHandler final /*: public RequestHandler::Service*/ {
	log::Logger* logger_;

public:
	explicit RequestHandler(log::Logger* logger);
	void shutdown();

	void on_message(const spark::Link& link, const spark::Message& message);
	void on_link_up(const spark::Link& link);
	void on_link_down(const spark::Link& link);
};

} // dns, ember