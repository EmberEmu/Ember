/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../ServiceContext.h"
#include <commands/Commands.h>
#include <logger/LoggerFwd.h>

namespace ember::realm {

void add_shutdown_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger);

} // realm, ember