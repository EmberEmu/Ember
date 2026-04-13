/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientHandler.h"
#include "ClientContextBuilder.h"

namespace ember::realm {

class ClientHandlerBuilder final {
	ClientContextBuilder builder_;
	EventDispatcher& dispatcher_;
	log::Logger& logger_;

public:
	ClientHandlerBuilder(ClientContextBuilder builder, EventDispatcher& dispatcher, log::Logger& logger)
		: builder_(builder)
		, dispatcher_(dispatcher)
		, logger_(logger) {}

	ClientHandler create(std::size_t index, executor executor) const {
		return ClientHandler(
			index, builder_.create(executor), dispatcher_, logger_
		);
	}
};

} // realm, ember
