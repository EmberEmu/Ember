/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "WorldRPCServer.h"
#include <spark/Server.h>
#include <dbcreader/Storage.h>
#include <memory>

namespace ember::world {

struct ServiceContext::Impl {
	std::unique_ptr<dbc::Storage> dbcs;
	std::unique_ptr<spark::Server> spark;
	std::unique_ptr<WorldRPCServer> world_rpc_service;
};

} // character, ember