/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <functional>

namespace ember::spark::inline v1 {

struct Link;
struct Message;

using Verifier = std::function<bool(const Message&)>;
using Handler = std::function<void(const Link&, const Message&)>;

struct LocalDispatcher {
	Verifier verify;
	Handler handle;
};

} // spark, ember

#define VERIFY(type, message) \
	[](auto& message) { return spark::Service::verify<type>(message); }

#define HANDLER(handler) \
	std::bind(&handler, this, std::placeholders::_1, std::placeholders::_2)

#define REGISTER(opcode, type, handler) \
	handlers_.emplace(opcode, spark::LocalDispatcher { VERIFY(type, message), HANDLER(handler) })
