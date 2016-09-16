/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/PatchBase.h>
#include <conpool/ConnectionPool.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <cppconn/prepared_statement.h>
#include <memory>

namespace ember { namespace dal {

using namespace std::chrono_literals;

template<typename T>
class PatchDAO final : public PatchBase {
	T& pool_;
	drivers::MySQL* driver_;

public:
	PatchDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

};

template<typename T>
std::unique_ptr<PatchDAO<T>> patch_dao(T& pool) {
	return std::make_unique<PatchDAO<T>>(pool);
}

}} //dal, ember
