/*
 * Copyright (c) 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
 
#pragma once

#include "DatabaseDetails.h"
#include <string>
#include <vector>

class QueryExecutor {
public:
	virtual ~QueryExecutor() { }
	virtual bool test_connection() = 0;
	virtual void create_user(const std::string& username, const std::string& password, bool drop) = 0;
	virtual void create_database(const std::string& name, bool drop) = 0;
	virtual void grant_user(const std::string& user, const std::string& db, bool read_only) = 0;
	virtual void execute(const std::string& query) = 0;
	virtual DatabaseDetails details() = 0;
	virtual std::vector<Migration> migrations() = 0;
	virtual void select_db(const std::string& schema) = 0;
	virtual void start_transaction() = 0;
	virtual void end_transaction() = 0;
	virtual void rollback() = 0;
	virtual void insert_migration_meta(const Migration& meta) = 0;
};