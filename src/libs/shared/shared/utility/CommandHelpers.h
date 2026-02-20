/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>

namespace ember {

namespace log { class Logger; }
namespace commands { class Registry; class Command;  }

} // ember

namespace ember::utility {

void register_shared_commands(commands::Registry& registry, log::Logger& logger);
void register_command_handlers(commands::Registry& registry, log::Logger& logger);

} // commands, ember