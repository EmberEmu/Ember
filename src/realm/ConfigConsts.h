/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>

using namespace std::chrono_literals;

namespace ember::realm::config {

constexpr static inline auto broadcast_timer_frequency = 30s;
constexpr static inline auto session_collect_frequency = 15s;

} // config, realm, ember