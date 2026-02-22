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
	const std::string prefix_;

public:
	PrefixedRegistry(Registry& registry, std::string prefix = {})
		: registry_(registry),
		  prefix_(std::move(prefix)) {}

	std::shared_ptr<Command> operator()(const std::string_view name) {
		return registry_.insert(prefix_ + std::string(name));
	}

	bool erase(const std::shared_ptr<const Command> command) {
		return registry_.erase(command);
	}
};

} // commands, ember