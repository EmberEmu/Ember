/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Command.h>
#include <string>
#include <span>
#include <unordered_map>
#include <vector>

namespace ember::blaze {

class PluginCommandRegistry final {
	std::unordered_map<std::string, std::vector<std::shared_ptr<commands::Command>>> mapping_;

public:
	void insert(std::string_view plugin, std::shared_ptr<commands::Command> command);
	bool remove(commands::Command* command);
	int remove(std::string_view plugin);
	std::shared_ptr<commands::Command> lookup(commands::Command* command);
	std::shared_ptr<commands::Command> lookup(std::string_view plugin, commands::Command* command);
	std::span<std::shared_ptr<commands::Command>> lookup(std::string_view plugin);
};

} // blaze, ember