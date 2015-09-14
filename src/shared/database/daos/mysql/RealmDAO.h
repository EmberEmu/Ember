/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/RealmBase.h>
#include <conpool/ConnectionPool.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <cppconn/prepared_statement.h>
#include <memory>

namespace ember { namespace dal { 

template<typename T>
class MySQLRealmDAO : public RealmDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLRealmDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::vector<Realm> get_realms() const override final try {
		std::string query = "SELECT id, name, ip, icon, flags, timezone, population FROM realms";

		auto conn = pool_.get_connection();
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<Realm> realms;

		while(res->next()) {
			// todo - temp fix to workaround msvc crash
			Realm temp{res->getUInt("id"), res->getString("name"), res->getString("ip"),
			           static_cast<float>(res->getDouble("population")),
		               static_cast<std::uint8_t>(res->getUInt("icon")),
					   static_cast<std::uint8_t>(res->getUInt("flags")),
		               static_cast<std::uint8_t>(res->getUInt("timezone"))};
			realms.emplace_back(std::move(temp));
		}

		return realms;
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	boost::optional<Realm> get_realm(int id) const override final try {
		std::string query = "SELECT id, name, ip, icon, flags, timezone, population FROM realms "
		                    "WHERE id = ?";

		auto conn = pool_.get_connection();
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setInt(1, id);

		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			// todo - temp fix to workaround msvc crash
			Realm temp{res->getUInt("id"), res->getString("name"), res->getString("ip"),
			           static_cast<float>(res->getDouble("population")),
		               static_cast<std::uint8_t>(res->getUInt("icon")),
					   static_cast<std::uint8_t>(res->getUInt("flags")),
		               static_cast<std::uint8_t>(res->getUInt("timezone"))};
			return boost::optional<Realm>(std::move(temp));
		}

		return boost::optional<Realm>();
	} catch(std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
std::unique_ptr<MySQLRealmDAO<T>> realm_dao(T& pool) {
	return std::make_unique<MySQLRealmDAO<T>>(pool);
}

}} //dal, ember