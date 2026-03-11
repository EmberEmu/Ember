/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CharacterService.h"
#include "CharacterHandler.h"
#include <dbcreader/Storage.h>
#include <conpool/ConnectionPool.h>
#include <conpool/drivers/AutoSelect.h>
#include <shared/database/daos/CharacterDAO.h>
#include <thread/ThreadPool.h>
#include <spark/Server.h>
#include <memory>

namespace ember::character {

struct ServiceContext::Impl {
	std::unique_ptr<dbc::Storage> dbcs;
	std::unique_ptr<thread::ThreadPool> thread_pool;
	std::unique_ptr<connection_pool::Pool<drivers::DriverType>> conn_pool;
	std::unique_ptr<dal::CharacterDAO> character_dao;
	std::unique_ptr<spark::Server> spark;
	std::unique_ptr<CharacterHandler> character_handler;
	std::unique_ptr<CharacterService> character_service;
};

} // character, ember