/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <protocol/MovementInfo.h>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::client {

struct MoveFallLand final {
	MovementInfo info;

	StreamResult read_from_stream(auto& stream) try {
		stream >> info.flags;
		stream >> info.timestamp;
		stream >> info.position;
		stream >> info.orientation;

		if(info.flags & MovementFlags::on_transport) {
			throw std::runtime_error("Not implemented, at all");
		}

		if(info.flags & MovementFlags::swimming) {
			stream >> info.pitch;
		}

		stream >> info.fall_time;

		if(info.flags & MovementFlags::jumping) {
			stream >> info.z_speed;
			stream >> info.cos_angle;
			stream >> info.sin_angle;
			stream >> info.xy_speed;
		}

		if(info.flags & MovementFlags::spline_elevation) {
			stream >> info.spline_elevation;
		}

		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(auto& stream) const try {
		stream << info.flags;
		stream << info.timestamp;
		stream << info.position;
		stream << info.orientation;

		if(info.flags & MovementFlags::on_transport) {
			throw std::runtime_error("Not implemented, at all");
		}

		if(info.flags & MovementFlags::swimming) {
			stream << info.pitch;
		}

		stream << info.fall_time;

		if(info.flags & MovementFlags::jumping) {
			stream << info.z_speed;
			stream << info.cos_angle;
			stream << info.sin_angle;
			stream << info.xy_speed;
		}

		if(info.flags & MovementFlags::spline_elevation) {
			stream << info.spline_elevation;
		}

		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // client, protocol, ember
