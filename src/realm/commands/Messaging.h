/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../EventDispatcher.h"
#include "../ServiceContextImpl.h"
#include "../SessionManager.h"
#include <commands/Commands.h>
#include <logger/LoggerFwd.h>
#include <shared/utility/CommandExecutor.h>

namespace ember::realm {

commands::ScopedCommand add_message_commands(
	commands::Command& registry,
	utility::CommandExecutor& exec,
    const EventDispatcher& dispatcher,
    const SessionManager& sessions,
	log::Logger& logger
);

} // realm, ember