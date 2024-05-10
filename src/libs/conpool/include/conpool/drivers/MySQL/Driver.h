/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/StringHash.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>
#include <cstdint>

namespace sql {

class Driver;
class Connection;
class PreparedStatement;

} // sql

namespace ember::drivers {

class MySQL final {
	using StmtPtr = std::unique_ptr<
		sql::PreparedStatement, std::function<void(sql::PreparedStatement*)>
	>;

	using QueryCache = std::unordered_map<std::string, StmtPtr,
		StringHash, std::equal_to<>>;

	const std::string dsn, username, password, database;
	sql::Driver* driver;
	mutable std::unordered_map<const sql::Connection*, QueryCache> cache_;
	mutable std::mutex cache_lock_;

	QueryCache* locate_cache(const sql::Connection* conn) const;
	void close_cache(const sql::Connection* conn) const;

public:
	MySQL(std::string user, std::string password, const std::string& host, std::uint16_t port,
	      std::string db = "");

	MySQL(MySQL&& rhs) noexcept : dsn(rhs.dsn), username(rhs.username),
	                              password(rhs.password), database(rhs.database),
	                              cache_(std::move(rhs.cache_)), driver(rhs.driver) { }

	MySQL& operator=(MySQL&&) = delete;
	MySQL& operator=(MySQL&) = delete;
	MySQL(MySQL&) = delete;

	static std::string name();
	static std::string version();

	sql::Connection* open() const;
	bool clean(sql::Connection* conn) const;
	void close(sql::Connection* conn) const;
	bool keep_alive(sql::Connection* conn) const;
	void thread_enter() const;
	void thread_exit() const;
	sql::PreparedStatement* prepare_cached(sql::Connection* conn, std::string_view key);
	sql::PreparedStatement* lookup_statement(const sql::Connection* conn, std::string_view key);
	void cache_statement(const sql::Connection* conn, std::string_view key,
	                     sql::PreparedStatement* value);
};

} // drivers, ember