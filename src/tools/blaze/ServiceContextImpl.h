/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ServiceContext.h"

#include "PluginRegistry.h"
#include "PluginCommands.h"
#include <commands/Command.h>
#include <logger/LoggerFwd.h>
#include <memory>
#include <vector>

namespace ember::blaze {

struct ServiceContext::Impl {
	std::unique_ptr<PluginRegistry> plugins;
	std::unique_ptr<PluginCommands> plugin_commands;
	commands::Command* commands; // temporary
	log::Logger* logger;
};

} // blaze, ember