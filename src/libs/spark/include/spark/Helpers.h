/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Service.h> // todo, remove

namespace ember { namespace spark {

typedef std::function<bool(const Message&)> Verifier;
typedef std::function<void(const Message&)> Handler;

struct LocalDispatcher {
	Verifier verify;
	Handler handle;
};

}} // spark, ember

#define VERIFY(type, message) \
	[](auto& message) { Service::verify<type>(message) }

#define HANDLER(type) \
	std::bind(&type, this, _1)

#define REGISTER(opcode, type, handler) \
	handlers_.emplace(opcode, VERIFY(type, ##message), HANDLER(handler))
