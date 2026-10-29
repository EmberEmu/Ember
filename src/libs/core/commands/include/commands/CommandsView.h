/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/CommandMap.h>
#include <mutex>

namespace ember::commands {

class CommandsView {
	std::unique_lock<std::recursive_mutex> guard_;
	const CommandMap* map_;

public:
	CommandsView(const CommandMap& map, std::recursive_mutex& lock)
		: guard_(lock)
		, map_(&map) {}

	CommandsView(const CommandsView&) = delete;
	CommandsView& operator=(const CommandsView&) = delete;

	CommandsView(CommandsView&&) noexcept = default;
	CommandsView& operator=(CommandsView&&) noexcept = default;

	const CommandMap* operator->() const {
		return map_;
	}

	auto begin() const { return map_->begin(); }
	auto end() const { return map_->end(); }
};

} // commands, ember
