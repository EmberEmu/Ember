/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/PatchBase.h>
#include <botan/bigint.h>
#include <conpool/ConnectionPool.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include <sstream>
#include <string_view>
#include <string>
#include <vector>

namespace ember::dal {

using namespace std::chrono_literals;

template<typename T>
class MySQLPatchDAO final : public PatchDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLPatchDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::vector<PatchMeta> fetch_patches() const override try {
		std::string_view query = "SELECT patches.id, `from`, `to`, mpq, name, size, md5, os, rollup, "
		                         "architecture, locale, os.value AS os_val, "
		                         "arch.value AS architecture_val, l.value AS locale_val "
		                         "FROM patches "
		                         "LEFT JOIN architectures arch ON patches.architecture = arch.id "  
		                         "LEFT JOIN locales l ON patches.locale = l.id "
		                         "LEFT JOIN operating_systems os ON patches.os = os.id";

		auto conn = pool_.try_acquire_for(60s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<PatchMeta> patches;

		while(res->next()) {
			PatchMeta meta{};
			meta.id = res->getUInt("id");
			meta.build_from = res->getUInt("from");
			meta.build_to = res->getUInt("to");
			meta.os_id = res->getUInt("os");
			meta.arch_id = res->getUInt("architecture");
			meta.locale_id = res->getUInt("locale");
			meta.mpq = res->getBoolean("mpq");
			meta.file_meta.name = res->getString("name");
			meta.file_meta.size = res->getUInt64("size");
			meta.os = res->getString("os_val");
			meta.locale = res->getString("locale_val");
			meta.arch = res->getString("architecture_val");
			meta.rollup = res->getBoolean("rollup");

			// sigh.
			auto md5 = res->getString("MD5");
			Botan::BigInt md5_int(reinterpret_cast<const std::uint8_t*>(md5.c_str()), md5.length(),
			                      Botan::BigInt::Base::Hexadecimal);
			Botan::BigInt::encode_1363(reinterpret_cast<std::uint8_t*>(meta.file_meta.md5.data()),
			                           meta.file_meta.md5.size(), md5_int);
			patches.emplace_back(std::move(meta));
		}

		return patches;
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void update(const PatchMeta& meta) const override try {
		std::string_view query = "UPDATE patches SET `from` = ?, `to` = ?, `mpq` = ?, "
		                         "`name` = ?, `size` = ?, `md5` = ?, `locale` = ?, "
		                         "`architecture` = ?, `os` = ?, `rollup` = ? "
		                         "WHERE id = ?";

		auto conn = pool_.try_acquire_for(60s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);

		stmt->setUInt(1, meta.build_from);
		stmt->setUInt(2, meta.build_to);
		stmt->setBoolean(3, meta.mpq);
		stmt->setString(4, meta.file_meta.name);
		stmt->setUInt64(5, meta.file_meta.size);
		auto md5 = Botan::BigInt::decode(reinterpret_cast<const std::uint8_t*>(meta.file_meta.md5.data()),
		                                 meta.file_meta.md5.size());
		std::stringstream md5_str;
		md5_str << std::hex << md5;
		stmt->setString(6, md5_str.str());
		stmt->setUInt(7, meta.locale_id);
		stmt->setUInt(8, meta.arch_id);
		stmt->setUInt(9, meta.os_id);
		stmt->setBoolean(10, meta.rollup);
		stmt->setUInt(11, meta.id);

		if(!stmt->executeUpdate()) {
			throw exception("Unable to update patch #" + std::to_string(meta.id));
		}
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLPatchDAO<T> patch_dao(T& pool) {
	return MySQLPatchDAO<T>(pool);
}

} // dal, ember
