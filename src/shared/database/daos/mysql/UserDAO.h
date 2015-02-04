/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/UserBase.h>
#include <conpool/ConnectionPool.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <conpool/drivers/MySQLDriver.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include <iostream>

namespace ember { namespace dal { 

template<typename T>
class MySQLUserDAO : public UserDAO {
	T& pool_;

public:
	MySQLUserDAO(T& pool) : pool_(pool) { }

	boost::optional<User> user(const std::string& username) override final try {
		auto conn = pool_.get_connection();
		auto driver = pool_.get_driver();

		std::string query = "SELECT u.username, u.id, u.s, u.v, b.user_id as banned, "
		                    "s.user_id as suspended FROM users u "
		                    "LEFT JOIN bans b ON u.id = b.user_id "
		                    "LEFT JOIN suspensions s ON u.id = s.user_id "
		                    "WHERE username = ?";
	
		sql::PreparedStatement* stmt = driver->prepare_cached(*conn, query);
		stmt->setString(1, username);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			User user(res->getString("username"), res->getString("s"), res->getString("v"),
			          res->getBoolean("banned"), res->getBoolean("suspended"));
			return user;
		}

		return boost::optional<User>();
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	void record_last_login(const User& user, const std::string& ip) override final try {
		auto conn = pool_.get_connection();
		auto driver = pool_.get_driver();

		std::string query = "INSERT INTO login_history (user_id, ip) VALUES "
		                    "((SELECT id AS user_id FROM users WHERE username = ?), ?)";
		sql::PreparedStatement* stmt = driver->prepare_cached(*conn, query);
		stmt->setString(1, user.username());
		stmt->setString(2, ip);
		
		if(!stmt->executeUpdate()) {
			throw exception("Unable to set last login for " + user.username());
		}
	} catch(std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
std::unique_ptr<MySQLUserDAO<T>> user_dao(T& pool) {
	return std::make_unique<MySQLUserDAO<T>>(pool);
}

}} //dal, ember