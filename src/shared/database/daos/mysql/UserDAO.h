/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <conpool/drivers/MySQLDriver.h>
#include <shared/database/daos/shared_base/UserBase.h>
#include <conpool/ConnectionPool.h>
#include <memory>

namespace ember { namespace dal {

template<typename T>
class MySQLUserDAO : public UserDAO {
	T& pool_;

public:
	MySQLUserDAO(T& pool) : pool_(pool) { }

	void read() override final {

	}
};

template<typename T>
std::unique_ptr<MySQLUserDAO<T>> user_dao(T& pool) {
	return std::make_unique<MySQLUserDAO<T>>(pool);
}

}} //dal, ember