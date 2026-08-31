/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PluginCommands.h"

namespace ember::blaze {

void PluginCommands::insert(std::string_view plugin, std::shared_ptr<commands::Command> command) {
	//
}

bool PluginCommands::remove(commands::Command* command) {
	return true;
}

int PluginCommands::remove(std::string_view plugin) {
	return 0;
}

std::shared_ptr<commands::Command> PluginCommands::lookup(std::string_view plugin, commands::Command* command) {
	return nullptr;
}

std::shared_ptr<commands::Command> PluginCommands::lookup(commands::Command* command) {
	return nullptr;
}

std::span<std::shared_ptr<commands::Command>> PluginCommands::lookup(std::string_view plugin) {
	return {};
}

} // blaze, ember