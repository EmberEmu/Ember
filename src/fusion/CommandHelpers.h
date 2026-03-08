/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <commands/Registry.h>

namespace ember {

namespace log { class Logger; }

} // ember

using Registries = std::unordered_map<std::string, ember::commands::Registry>;

namespace ember::fusion {

void register_shared_commands(Registries& registry, log::Logger& logger);
void register_command_handlers(Registries& registry, log::Logger& logger);

} // fusion, ember