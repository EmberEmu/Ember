/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PluginCommandRegistry.h"
#include <commands/Commands.h>
#include <logger/LoggerFwd.h>

namespace ember::blaze {

class InterfaceContainer final {
	InterfaceContainer() = default;

	log::Logger* logger_ = nullptr;
	commands::Command* command_root_ = nullptr;
	PluginCommandRegistry* pcr_ = nullptr;

public:
	static InterfaceContainer& get_instance() {
		static InterfaceContainer instance;
		return instance;
	}

	log::Logger* logger() {
		return logger_;
	}

	commands::Command* command_root() {
		return command_root_;
	}

	PluginCommandRegistry* plugin_command_registry() {
		return pcr_;
	}

	void logger(log::Logger* logger) {
		logger_ = logger;
	}

	void command_root(commands::Command* command_root) {
		command_root_ = command_root;;
	}

	void plugin_command_registry(PluginCommandRegistry* pcr) {
		pcr_ = pcr;
	}
};

} // blaze, ember