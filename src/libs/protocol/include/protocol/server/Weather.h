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
#include <array>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

struct Weather final {
	enum WeatherType : std::uint32_t {
		FINE = 0,
		RAIN = 1,
		SNOW = 2,
		STORM = 3
	};

	enum WeatherChangeType : std::uint32_t {
		SMOOTH = 0,
		INSTANT = 1
	};

	WeatherType type;
	float grade;
	std::uint32_t sound_id;
	WeatherChangeType change;

	StreamResult read_from_stream(auto& stream) try {
		stream >> type;
		stream >> grade;
		stream >> sound_id;
		stream >> change;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(auto& stream) const try {
		stream << type;
		stream << grade;
		stream << sound_id;
		stream << change;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // server, protocol, ember
