/*
 * Copyright (c) 2015, 2016 Ember
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
#include <conpool/drivers/MySQL/Driver.h>
#include <cppconn/prepared_statement.h>
#include <chrono>
#include <memory>

namespace ember { namespace dal { 

using namespace std::chrono_literals;

template<typename T>
class MySQLUserDAO final : public UserDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLUserDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	boost::optional<User> user(const std::string& username) const override try {
		const std::string query = "SELECT u.username, u.id, u.s, u.v, b.user_id as banned, "
		                          "s.user_id as suspended FROM users u "
		                          "LEFT JOIN bans b ON u.id = b.user_id "
		                          "LEFT JOIN suspensions s ON u.id = s.user_id "
		                          "WHERE username = ?";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, username);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			User user(res->getUInt("id"), res->getString("username"), res->getString("s"),
			          res->getString("v"), res->getBoolean("banned"), res->getBoolean("suspended"));
			return user;
		}

		return boost::optional<User>();
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	void record_last_login(const User& user, const std::string& ip) const override try {
		const std::string query = "INSERT INTO login_history (user_id, ip) VALUES "
		                          "((SELECT id AS user_id FROM users WHERE username = ?), ?)";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, user.username());
		stmt->setString(2, ip);
		
		if(!stmt->executeUpdate()) {
			throw exception("Unable to set last login for " + user.username());
		}
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	std::unordered_map<std::uint32_t, std::uint32_t> character_counts(std::uint32_t user_id) const override try {
		const std::string query = "SELECT COUNT(c.id) AS count, c.realm_id "
		                          "FROM users u, characters c "
		                          "WHERE u.id = ? GROUP BY c.realm_id";
		
		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt(1, user_id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		std::unordered_map<std::uint32_t, std::uint32_t> counts;

		if(res->next()) {
			counts.emplace(res->getUInt("realm_id"), res->getUInt("count"));
		}

		return counts;
	} catch(std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
std::unique_ptr<MySQLUserDAO<T>> user_dao(T& pool) {
	return std::make_unique<MySQLUserDAO<T>>(pool);
}

}} //dal, ember