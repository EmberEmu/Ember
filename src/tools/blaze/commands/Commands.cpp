/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Commands.h"
#include "PluginControl.h"

namespace ember::blaze {

void install_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	install_plugin_commands(context, registry, logger);
}

} // blaze, ember