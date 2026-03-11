/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <string_view>

namespace ember::world {

inline const std::array<std::string_view, 2> dbcs_required {
	"Map",
	"GameTips"
};

} // world