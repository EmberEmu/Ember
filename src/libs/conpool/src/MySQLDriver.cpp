/*
 * Copyright (c) 2014, 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/MySQLDriver.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include <sstream>

namespace ember { namespace drivers {

MySQL::MySQL(std::string user, std::string pass, const std::string& host, unsigned short port,
             std::string db) : database(db), username(std::move(user)), password(std::move(pass)),
             dsn(std::string("tcp://" + host + ":" + std::to_string(port))) {
	driver = get_driver_instance();
}

sql::Connection* MySQL::open() const {
	sql::Connection* conn = driver->connect(dsn, username, password);
	conn->setSchema(database);
	conn->setAutoCommit(true);
	bool opt = true;
	conn->setClientOption("MYSQL_OPT_RECONNECT", &opt);
	return conn;
}

void MySQL::close(sql::Connection* conn) const {
	if(!conn->isClosed()) {
		conn->close();
	}

	auto conn_cache = locate_cache(conn);

	if(conn_cache) {
		for(auto stmt : *conn_cache) {
			stmt.second->close();
			delete stmt.second;
		}

		close_cache(conn);
	}

	delete conn;
}

//todo - change this interface to bool
sql::Connection* MySQL::keep_alive(sql::Connection* conn) const try {
	conn->setAutoCommit(true);
	std::unique_ptr<sql::Statement> stmt(conn->createStatement());
	stmt->execute("/* ping */");
	return conn;
} catch(sql::SQLException&) {
	return open();
}

bool MySQL::clean(sql::Connection* conn) const {
	return !conn->isClosed();
}

void MySQL::thread_enter() const {
	driver->threadInit();
}

void MySQL::thread_exit() const {
	driver->threadEnd();
}

std::string MySQL::name() {
	auto driver = get_driver_instance();
	return driver->getName();
}

std::string MySQL::version() {
	auto driver = get_driver_instance();
	std::stringstream ver;
	ver << driver->getMajorVersion() << "." << driver->getMinorVersion() << "."
	    << driver->getPatchVersion();
	return ver.str();
}

sql::PreparedStatement* MySQL::prepare_cached(sql::Connection* conn, const std::string& key) {
	auto stmt = lookup_statement(conn, key);
	
	if(!stmt) {
		stmt = conn->prepareStatement(key);
		cache_statement(conn, key, stmt);
	}

	return stmt;
}

sql::PreparedStatement* MySQL::lookup_statement(const sql::Connection* conn, const std::string& key) {
	auto conn_cache = locate_cache(conn);

	if(!conn_cache) {
		return nullptr;
	}

	auto conn_cache_it = conn_cache->find(key);
	
	if(conn_cache_it == conn_cache->end()) {
		return nullptr;
	}

	return conn_cache_it->second;
}

void MySQL::cache_statement(const sql::Connection* conn, const std::string& key,
                            sql::PreparedStatement* value) {
	std::lock_guard<std::mutex> lock(cache_lock_);
	cache_[conn][key] = value;
}

MySQL::QueryCache* MySQL::locate_cache(const sql::Connection* conn) const {
	std::lock_guard<std::mutex> lock(cache_lock_);
	auto cache_it = cache_.find(conn);

	if(cache_it == cache_.end()) {
		return nullptr;
	}

	return &cache_it->second;
}

void MySQL::close_cache(const sql::Connection* conn) const {
	std::lock_guard<std::mutex> lock(cache_lock_);
	auto cache_it = cache_.find(conn);
	
	if(cache_it != cache_.end()) {
		cache_.erase(cache_it);
	}
}

}} // drivers, ember