/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AccountClient.h"
#include "CharacterClient.h"
#include "BroadcastTimer.h"
#include "Config.h"
#include "ConfigStore.h"
#include "EventDispatcher.h"
#include "NetworkListener.h"
#include "RealmService.h"
#include "RealmQueue.h"
#include "ServiceContext.h"
#include "SessionManager.h"
#include "WorldRPCClient.h"
#include <commands/ScopedCommand.h>
#include <dbcreader/Storage.h>
#include <nsd/NSD.h>
#include <ports/Forward.h>
#include <shared/utility/CommandExecutor.h>
#include <shared/Realm.h>
#include <spark/Server.h>
#include <thread/ServicePool.h>
#include <memory>
#include <vector>

namespace ember::realm {

struct ServiceContext::Impl {
	Realm realm;
	SessionManager sessions;
	thread::ServicePool* service_pool = nullptr;
	std::unique_ptr<utility::CommandExecutor> cmd_exec;
	std::unique_ptr<ConfigStore> config_store;
	std::unique_ptr<ports::Forward> port_daemon;
	std::unique_ptr<EventDispatcher> dispatcher;
	std::unique_ptr<dbc::Storage> dbcs;
	std::unique_ptr<RealmQueue> queue;
	std::unique_ptr<spark::Server> rpc;
	std::unique_ptr<AccountClient> rpc_account;
	std::unique_ptr<CharacterClient> rpc_character;
	std::unique_ptr<RealmService> rpc_realm;
	std::unique_ptr<WorldRPCClient> rpc_world;
	std::unique_ptr<NetworkServiceDiscovery> rpc_discovery;
	std::unique_ptr<NetworkListener> server;
	std::unique_ptr<BroadcastTimer> timer;
	std::vector<commands::ScopedCommand> commands;
};

} // realm, ember