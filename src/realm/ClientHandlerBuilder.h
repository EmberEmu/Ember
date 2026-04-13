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
#include <shared/ClientIdent.h>

namespace ember::realm {

class ClientHandlerBuilder final {
	ClientContextBuilder builder_;
	log::Logger& logger_;

public:
	ClientHandlerBuilder(ClientContextBuilder builder, log::Logger& logger)
		: builder_(builder)
		, logger_(logger) {}

	ClientHandler create(ClientIdent ident, executor executor) const {
		return ClientHandler(ident, builder_.create(executor), logger_);
	}
};

} // realm, ember
