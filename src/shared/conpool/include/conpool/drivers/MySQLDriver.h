/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace sql {

class Driver;
class Connection;
class PreparedStatement;

}

namespace ember { namespace drivers {

typedef std::unordered_map<std::string, sql::PreparedStatement*> QueryCache;

class MySQL {
	

	const std::string dsn, username, password, database;
	sql::Driver* driver;
	mutable std::unordered_map<const sql::Connection*, QueryCache> cache_;
	mutable std::mutex cache_lock_;

public:
	MySQL(std::string user, std::string password, const std::string& host,
	      unsigned short port, std::string db);
	
	sql::Connection* open() const;
	sql::Connection* clean(sql::Connection* conn) const;
	void close(sql::Connection* conn) const;
	void clear_state(sql::Connection* conn) const;
	sql::Connection* keep_alive(sql::Connection* conn) const;
	void thread_enter() const;
	void thread_exit() const;
	std::string name() const;
	std::string version() const;
	QueryCache* locate_cache(const sql::Connection* conn) const;
	sql::PreparedStatement* lookup_statement(const sql::Connection* conn, const std::string& key) const;
	void cache_statement(const sql::Connection* conn, const std::string& key,
	                     sql::PreparedStatement* value);
};

}} //drivers, ember