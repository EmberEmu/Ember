/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Command.h>
#include <any>
#include <memory>
#include <typeinfo>

namespace ember {

namespace log { 
	class Logger;
}

} // ember

namespace ember::utility {

std::any convert_type(const std::type_info& type, std::string_view token);
void register_common_commands(std::shared_ptr<commands::Command> root, log::Logger& logger);
void register_command_handlers(std::shared_ptr<commands::Command> root, log::Logger& logger, bool suggestions);

} // commands, ember