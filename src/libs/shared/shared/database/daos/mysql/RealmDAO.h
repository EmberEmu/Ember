/*
 * Copyright (c) 2015 - 2024 Ember
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
#include <string_view>

namespace ember::dal { 

using namespace std::chrono_literals;

template<typename T>
class MySQLRealmDAO final : public RealmDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLRealmDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::vector<Realm> get_realms() const override final try {
		const std::string_view query = "SELECT id, name, ip, type, flags, category, "
		                               "region, creation_setting, population FROM realms";

		auto conn = pool_.try_acquire_for(60s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<Realm> realms;

		while(res->next()) {
			realms.emplace_back(
				res->getUInt("id"), res->getString("name"), res->getString("ip"),
				static_cast<float>(res->getDouble("population")),
				static_cast<Realm::Type>(res->getUInt("type")),
				static_cast<Realm::Flags>(res->getUInt("flags")),
				static_cast<dbc::Cfg_Categories::Category>(res->getUInt("category")),
				static_cast<dbc::Cfg_Categories::Region>(res->getUInt("region")),
				static_cast<Realm::CreationSetting>(res->getUInt("creation_setting"))
			);
		}

		return realms;
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::optional<Realm> get_realm(std::uint32_t id) const override final try {
		const std::string_view query = "SELECT id, name, ip, type, flags, category, "
		                               "region, creation_setting, population FROM realms "
		                               "WHERE id = ?";
	
		auto conn = pool_.try_acquire_for(60s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setInt(1, id);

		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			return Realm {
				res->getUInt("id"), res->getString("name"), res->getString("ip"),
			    static_cast<float>(res->getDouble("population")),
		        static_cast<Realm::Type>(res->getUInt("type")),
				static_cast<Realm::Flags>(res->getUInt("flags")),
			    static_cast<dbc::Cfg_Categories::Category>(res->getUInt("category")),
		        static_cast<dbc::Cfg_Categories::Region>(res->getUInt("region")),
			    static_cast<Realm::CreationSetting>(res->getUInt("creation_setting"))
			};
		}

		return std::nullopt;
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLRealmDAO<T> realm_dao(T& pool) {
	return MySQLRealmDAO<T>(pool);
}

} // dal, ember