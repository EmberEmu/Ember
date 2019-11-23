/*
 * Copyright (c) 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
 
#pragma once

#include "../QueryExecutor.h"
#include <cppconn/connection.h>
#include <memory>

class MySQLQueryExecutor final : public QueryExecutor {
	const DatabaseDetails details_;
	std::unique_ptr<sql::Connection> conn_;

public:
	MySQLQueryExecutor(DatabaseDetails details);

	bool test_connection() override;
	void create_user(const std::string& username, const std::string& password, bool drop) override;
	void create_database(const std::string& name, bool drop_if_exists) override;
	void grant_user(const std::string& user, const std::string& db, bool read_only) override;
	void execute(const std::string& query) override;
	DatabaseDetails details() override;
	std::vector<Migration> migrations() override;
	void select_db(const std::string& schema) override;
	void start_transaction() override;
	void end_transaction() override;
	void rollback() override;
	void insert_migration_meta(const Migration& meta) override;
};