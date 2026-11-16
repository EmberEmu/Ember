/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Command.h>
#include <logger/LoggerFwd.h>
#include <any>
#include <filesystem>
#include <memory>
#include <typeinfo>

namespace ember::utility {

void register_common_commands(std::shared_ptr<commands::Command> root, log::Logger& logger);
void register_theme_commands(std::shared_ptr<commands::Command> root, log::Logger& logger, std::filesystem::path theme_path);

} // utility, ember