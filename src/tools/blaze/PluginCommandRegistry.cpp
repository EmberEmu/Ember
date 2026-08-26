/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PluginCommandRegistry.h"

namespace ember::blaze {

void PluginCommandRegistry::insert(std::string_view plugin, std::shared_ptr<commands::Command> command) {
	//
}

bool PluginCommandRegistry::remove(commands::Command* command) {
	//
}

int PluginCommandRegistry::remove(std::string_view plugin) {
	//
}

std::shared_ptr<commands::Command> PluginCommandRegistry::lookup(std::string_view plugin, commands::Command* command) {
	//
}

std::shared_ptr<commands::Command> PluginCommandRegistry::lookup(commands::Command* command) {
	//
}

std::span<std::shared_ptr<commands::Command>> PluginCommandRegistry::lookup(std::string_view plugin) {
	//
}

} // blaze, ember