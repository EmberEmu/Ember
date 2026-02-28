/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AccountHandler.h"
#include "AccountService.h"
#include "Sessions.h"
#include <thread/ThreadPool.h>
#include <conpool/ConnectionPool.h>
#include <conpool/drivers/AutoSelect.h>
#include <shared/database/daos/UserDAO.h>
#include <spark/Server.h>
#include <memory>

namespace ember::account {

struct ServiceContext::Impl {
	std::unique_ptr<thread::ThreadPool> thread_pool;
	std::unique_ptr<connection_pool::Pool<drivers::DriverType>> conn_pool;
	std::unique_ptr<dal::UserDAO> user_dao;
	std::unique_ptr<AccountHandler> account_handler;
	std::unique_ptr<Sessions> sessions;
	std::unique_ptr<spark::Server> spark;
	std::unique_ptr<AccountService> account_service;
};

} // account, ember