/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Multicast_generated.h"
#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>
#include <functional>
#include <cstdint>

namespace ember { namespace spark {

struct Link;
struct Endpoint;
class ServiceDiscovery;

typedef boost::optional<boost::uuids::uuid> Beacon;

struct Message {
	messaging::Service service;
	std::uint16_t opcode;
	std::uint32_t size;
	const std::uint8_t* data;
	Beacon token;
};

typedef std::function<void(const spark::Link&, boost::optional<Message>)> TrackingHandler;
typedef std::function<void(const Endpoint*)> ResolveCallback;
typedef std::function<void(const messaging::multicast::LocateAnswer*)> LocateCallback;

}} // spark, ember