/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AccountClient.h"
#include "IntegrityData.h"
#include "LoginHandlerBuilder.h"
#include "NetworkListener.h"
#include "Patcher.h"
#include "RealmClient.h"
#include "RealmList.h"
#include "SessionBuilder.h"
#include "Survey.h"
#include <commands/ScopedCommand.h>
#include <conpool/ConnectionPool.h>
#include <conpool/drivers/AutoSelect.h>
#include <ports/Forward.h>
#include <shared/IPBanCache.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/MetricsPoll.h>
#include <shared/metrics/Monitor.h>
#include <shared/utility/CommandExecutor.h>
#include <spark/Server.h>
#include <thread/ThreadPool.h>
#include <memory>

namespace ember::login {

struct ServiceContext::Impl {
	std::unique_ptr<utility::CommandExecutor> cmd_exec;
	std::unique_ptr<thread::ThreadPool> thread_pool;
	std::unique_ptr<connection_pool::Pool<drivers::DriverType>> conn_pool;
	std::unique_ptr<dal::UserDAO> user_dao;
	std::unique_ptr<Monitor> monitor;
	std::unique_ptr<Metrics> metrics;
	std::unique_ptr<MetricsPoll> metrics_poll;
	std::unique_ptr<Patcher> patcher;
	std::unique_ptr<Survey> survey;
	std::unique_ptr<RealmList> realm_list;
	std::unique_ptr<IPBanCache> ip_ban_cache;
	std::unique_ptr<IntegrityData> integrity_data;
	std::unique_ptr<LoginHandlerBuilder> login_handler_builder;
	std::unique_ptr<LoginSessionBuilder> login_session_builder;
	std::unique_ptr<ports::Forward> port_daemon;
	std::unique_ptr<spark::Server> rpc;
	std::unique_ptr<AccountClient> rpc_account;
	std::unique_ptr<RealmClient> rpc_realm;
	std::unique_ptr<NetworkListener> server;
	std::vector<commands::ScopedCommand> commands;
};

} // login, ember