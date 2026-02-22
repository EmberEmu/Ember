/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>

namespace ember::commands {

class Command;

class ScopedCommand {
	friend class Command;

	std::shared_ptr<Command> command_;
	std::weak_ptr<Command> parent_;

	ScopedCommand(std::shared_ptr<Command> command, std::weak_ptr<Command> parent);

public:
	void reset();
	std::shared_ptr<Command> release();

	ScopedCommand(ScopedCommand&&) noexcept;
	ScopedCommand& operator=(ScopedCommand&&) noexcept;

	ScopedCommand(ScopedCommand&) = delete;
	ScopedCommand& operator=(ScopedCommand&) = delete;

	~ScopedCommand();
};

} // commands, ember