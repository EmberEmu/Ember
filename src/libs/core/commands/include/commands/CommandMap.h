/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/impl/StringHash.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace ember::commands {

class Command;

using CommandMap = std::unordered_map<std::string, std::shared_ptr<Command>, impl::StringHash, std::equal_to<>>;

} // commands, ember