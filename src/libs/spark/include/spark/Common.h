/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Services_generated.h"
#include <boost/uuid/uuid.hpp>
#include <functional>
#include <optional>
#include <cstdint>

namespace ember::spark::inline v1 {

struct Link;
struct Endpoint;

struct Beacon {
	bool tracked;
	boost::uuids::uuid uuid;
};

struct Message {
	std::uint32_t size;
	std::uint16_t opcode;
	messaging::Service service;
	const std::uint8_t* data;
	Beacon token;
};

using TrackingHandler = std::function<void(const Link&, std::optional<Message>)>;
using ResolveCallback = std::function<void(const Endpoint*)>;

} // spark, ember