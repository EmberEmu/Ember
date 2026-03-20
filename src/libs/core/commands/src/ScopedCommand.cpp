/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <commands/ScopedCommand.h>
#include <commands/Command.h>
#include <utility>

namespace ember::commands {

ScopedCommand::ScopedCommand(std::shared_ptr<Command> command, std::weak_ptr<Command> parent)
	: command_(std::move(command))
	, parent_(std::move(parent)) {}

ScopedCommand::ScopedCommand(ScopedCommand&& other) noexcept
	: command_(std::move(other.command_))
	, parent_(std::move(other.parent_)) {
	other.reset();
}

ScopedCommand::~ScopedCommand() {
	auto locked = parent_.lock();

	// parent doesn't exist or command released, no need to deregister
	if(!locked) {
		return;
	}

	locked->erase(command_);
}

ScopedCommand& ScopedCommand::operator=(ScopedCommand&& rhs) noexcept {
	if(this != &rhs) {
		reset();
		command_ = std::move(rhs.command_);
		parent_  = std::move(rhs.parent_);
	}

	return *this;
}

void ScopedCommand::reset() {
	if(auto locked = parent_.lock()) {
		locked->erase(command_);
	}

	command_.reset();
	parent_.reset();
}

std::shared_ptr<Command> ScopedCommand::release() {
	parent_.reset();
	return std::move(command_);
}

std::shared_ptr<Command> ScopedCommand::command() {
	return command_;
}

} // commands, ember