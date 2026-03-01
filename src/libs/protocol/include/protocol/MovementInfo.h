/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember::protocol {

enum MovementFlags : std::uint32_t {
	NONE               = 0x00000000,
	FORWARD            = 0x00000001,
	BACKWARD           = 0x00000002,
	STRAFE_LEFT        = 0x00000004,
	STRAFE_RIGHT       = 0x00000008,
	TURN_LEFT          = 0x00000010,
	TURN_RIGHT         = 0x00000020,
	PITCH_UP           = 0x00000040,
	PITCH_DOWN         = 0x00000080,
	WALK_MODE          = 0x00000100,
	ON_TRANSPORT       = 0x00000200,
	LEVITATING         = 0x00000400,
	FIXED_Z            = 0x00000800,
	ROOT               = 0x00001000,
	JUMPING            = 0x00002000,
	FALLINGFAR         = 0x00004000,
	SWIMMING           = 0x00200000,
	SPLINE_ENABLED     = 0x00400000,
	CAN_FLY            = 0x00800000,
	FLYING             = 0x01000000,
	ONTRANSPORT        = 0x02000000,
	SPLINE_ELEVATION   = 0x04000000,
	WATERWALKING       = 0x10000000,
	SAFE_FALL          = 0x20000000,
	HOVER              = 0x40000000
};

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

} // protocol, ember