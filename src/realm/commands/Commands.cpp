/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Commands.h"
#include "Connections.h"
#include "Message.h"
#include "Set.h"
#include "Ungrouped.h"

namespace ember::realm {

void install_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	add_ungrouped_commands(context, registry, logger);
	add_set_commands(context, registry, logger);
	add_connections_commands(context, registry, logger);
	add_message_commands(context, registry, logger);
}

} // realm, ember