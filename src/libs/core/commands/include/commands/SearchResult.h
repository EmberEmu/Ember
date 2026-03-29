/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <cstddef>

namespace ember::commands {

class Command;

struct SearchResult {
	std::shared_ptr<Command> command;
	std::size_t depth = 0;
};

} // commands, ember