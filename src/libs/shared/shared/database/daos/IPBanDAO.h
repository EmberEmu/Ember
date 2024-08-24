/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef DB_MYSQL
	#include <shared/database/daos/mysql/IPBanDAO.h>
	#define IP_BAN_DAO_IMPL MySQLIPBanDAO<_pool>
#elif DB_POSTGRESQL
 	#include <shared/database/daos/postgresql/IPBanDAO.h>
	#define IP_BAN_DAO_IMPL PostgresIPBanDAO<_pool>
#endif

namespace ember::dal {

template<typename _pool>
using IPBanDAOImpl = IP_BAN_DAO_IMPL;

} // dal, ember