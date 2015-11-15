/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/CoreHandler.h>
#include <spark/temp/Core_generated.h>

namespace ember { namespace spark {

CoreHandler::CoreHandler(log::Logger* logger, log::Filter filter)
                         : logger_(logger), filter_(filter) { }

void CoreHandler::handle_message(messaging::MessageRoot* message) {
	
}

}}