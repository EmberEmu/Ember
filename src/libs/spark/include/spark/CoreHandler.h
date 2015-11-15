/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <spark/temp/MessageRoot_generated.h>

namespace ember { namespace spark {

class CoreHandler {
	log::Logger* logger_;
	log::Filter filter_;

public:
	CoreHandler(log::Logger* logger, log::Filter filter);
	void handle_message(messaging::MessageRoot* message);
};

}}