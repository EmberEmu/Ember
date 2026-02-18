/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Registry.h"
#include <string>
#include <string_view>

namespace ember::commands {

class PrefixedRegistry {
	Registry& registry_;
	std::string prefix_;

public:
	PrefixedRegistry(Registry& registry, std::string prefix = {})
		: registry_(registry),
		  prefix_(std::move(prefix)) {}

	std::shared_ptr<Command> operator()(std::string_view name) {
		return registry_.register_command(prefix_ + std::string(name));
	}
};

} // commands, ember