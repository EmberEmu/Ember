/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Registry.h>

namespace ember {

namespace log { 
	class Logger;
}

} // ember

namespace ember::utility {

commands::args::Value convert_type(commands::args::Type type, std::string_view token);
void register_shared_commands(commands::Registry& registry, log::Logger& logger);
void register_command_handlers(commands::Registry& registry, log::Logger& logger);

} // commands, ember