/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef DB_MYSQL
	#include <shared/database/daos/mysql/UserDAO.h>
	#define USER_DAO_IMPL MySQLUserDAO<_pool>
#elif DB_POSTGRESQL
 	#include <shared/database/daos/postgresql/UserDAO.h>
	#define USER_DAO_IMPL PostgresUserDAO<_pool>
#endif

namespace ember::dal {

template<typename _pool>
using UserDAOImpl = USER_DAO_IMPL;

} // dal, ember