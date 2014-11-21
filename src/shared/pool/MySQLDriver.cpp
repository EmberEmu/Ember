/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "MySQLDriver.h"
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <memory>

MySQL::MySQL(const std::string& user, const std::string& pass, const std::string& host,
             unsigned short port, const std::string& db) : database(db), username(user),
             password(pass), dsn(std::string("tcp://" + host + ":" + std::to_string(port))) {
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
	conn->close();
}

sql::Connection* MySQL::keep_alive(sql::Connection* conn) const try {
	std::unique_ptr<sql::Statement> stmt(conn->createStatement());
	conn->setAutoCommit(true);
	stmt->execute("/* ping */");
	return conn;
} catch(sql::SQLException&) {
	return open();
}

sql::Connection* MySQL::clean(sql::Connection* conn) const {
	return conn;
}

void MySQL::thread_enter() const {
	driver->threadInit();
}

void MySQL::thread_exit() const {
	driver->threadEnd();
}