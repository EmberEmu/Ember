/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef DB_MYSQL
	#include <shared/database/daos/mysql/CharacterDAO.h>
	#define CHARACTER_DAO_IMPL MySQLCharacterDAO<_pool>
#elif DB_POSTGRESQL
 	#include <shared/database/daos/postgresql/CharacterDAO.h>
	#define CHARACTER_DAO_IMPL PostgresCharacterDAO<_pool>
#endif

namespace ember::dal {

template<typename _pool>
using CharacterDAOImpl = CHARACTER_DAO_IMPL;

} // dal, ember