/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <shared/smartenum.hpp>
#include <shared/utility/enum_bitmask.h>
#include <array>
#include <shared/utility/polyfill/inplace_vector>
#include <string_view>

namespace ember::protocol {

smart_enum(MovementFlags, std::uint32_t,
	none               = 0x00000000,
	forward            = 0x00000001,
	backward           = 0x00000002,
	strafe_left        = 0x00000004,
	strafe_right       = 0x00000008,
	turn_left          = 0x00000010,
	turn_right         = 0x00000020,
	pitch_up           = 0x00000040,
	pitch_down         = 0x00000080,
	walk_mode          = 0x00000100,
	on_transport       = 0x00000200,
	levitating         = 0x00000400,
	fixed_z            = 0x00000800,
	root               = 0x00001000,
	jumping            = 0x00002000,
	fallingfar         = 0x00004000,
	swimming           = 0x00200000,
	spline_enabled     = 0x00400000,
	can_fly            = 0x00800000,
	flying             = 0x01000000,
	ontransport        = 0x02000000,
	spline_elevation   = 0x04000000,
	waterwalking       = 0x10000000,
	safe_fall          = 0x20000000,
	hover              = 0x40000000
);

struct MovementInfo {
	MovementFlags flags;
	std::uint32_t timestamp;
	struct {
		float x, y, z;
	} position;
	float orientation;
	char transport; // dummy
	float pitch;
	float fall_time;
	float z_speed;
	float cos_angle;
	float sin_angle;
	float xy_speed;
	float spline_elevation;
};

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