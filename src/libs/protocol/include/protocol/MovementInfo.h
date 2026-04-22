/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <generated/types/MovementFlags.h>
#include <generated/types/MovementInfo.h>
#include <shared/utility/polyfill/inplace_vector>
#include <array>
#include <string_view>

namespace ember::protocol {

inline auto movement_flags_debug(const MovementFlags& flags) {
	constexpr std::array table {
		MovementFlags::forward,
		MovementFlags::backward,
		MovementFlags::strafe_left,
		MovementFlags::strafe_right,
		MovementFlags::turn_left,
		MovementFlags::turn_right,
		MovementFlags::pitch_up,
		MovementFlags::pitch_down,
		MovementFlags::walk_mode,
		MovementFlags::on_transport,
		MovementFlags::levitating,
		MovementFlags::fixed_z,
		MovementFlags::root,
		MovementFlags::jumping,
		MovementFlags::fallingfar,
		MovementFlags::swimming,
		MovementFlags::spline_enabled,
		MovementFlags::can_fly,
		MovementFlags::flying,
		MovementFlags::ontransport,
		MovementFlags::spline_elevation,
		MovementFlags::waterwalking,
		MovementFlags::safe_fall,
		MovementFlags::hover,
	};

	std::inplace_vector<std::string_view, table.size()> strings;

	if(flags == MovementFlags::none) {
		strings.emplace_back(MovementFlags_to_string(MovementFlags::none));
		return strings;
	}

	for(auto flag : table) {
		if((flags & flag) == flag) {
			strings.emplace_back(MovementFlags_to_string(flag));
		}
	}

	return strings;
}

} // protocol, ember
