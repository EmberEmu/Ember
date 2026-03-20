/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ArgumentType.h>
#include <array>
#include <typeindex>

namespace ember::commands::detail {

extern const std::array<std::type_index, 13> types;

} // detail, commands, ember