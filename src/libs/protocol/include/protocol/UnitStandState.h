/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
#include <shared/utility/polyfill/inplace_vector>
#include <array>
#include <string_view>
#include <cstdint>

namespace ember::protocol {

smart_enum(UnitStandState, std::uint8_t,
	stand = 0,
	sit = 1,
	sit_chair = 2,
	sleep = 3,
	sit_low_chair = 4,
	sit_medium_chair = 5,
	sit_high_chair = 6,
	dead = 7,
	kneel = 8,
	custom = 9
);

} // protocol, ember