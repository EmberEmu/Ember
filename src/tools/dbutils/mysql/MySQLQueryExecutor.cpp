/*
 * Copyright (c) 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
 
#include "MySQLQueryExecutor.h"
#include <conpool/drivers/MySQL/Driver.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include <sstream>

using namespace ember;

MySQLQueryExecutor::MySQLQueryExecutor(DatabaseDetails details) : details_(std::move(details)) {
	auto driver = drivers::MySQL(details_.username, details_.password, details_.hostname, details_.port);
	conn_ = std::unique_ptr<sql::Connection>(driver.open());
}

bool MySQLQueryExecutor::test_connection() {
	return conn_->isValid();
}

void MySQLQueryExecutor::create_user(const std::string& username, const std::string& password,
									 const bool drop) {
	if(drop) {
		if(username == "root") { // I made this mistake so you don't have to
			throw std::runtime_error("Cannot drop 'root' user, unless you want a broken database");
		}

		std::string query("DROP USER IF EXISTS " + username + "@'%'");
		const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
		stmt->execute(std::move(query));
	}
	
	std::stringstream query;
	query << "CREATE USER '" << username << "'@'%' IDENTIFIED BY '"
		  << password << "';";
	const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
	stmt->execute(query.str());   
}

void MySQLQueryExecutor::create_database(const std::string& name, bool drop) {
	if(drop) {
		const auto query = "DROP DATABASE IF EXISTS " + name + ";";
		const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
		stmt->execute(query);
	}

	const auto query = "CREATE DATABASE " + name + ";";
	const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
	stmt->execute(query);
}

void MySQLQueryExecutor::grant_user(const std::string& user, const std::string& db, bool read_only) {
	std::stringstream query;
	query << "GRANT SELECT";

	if(!read_only) {
		query << ", INSERT, DELETE, UPDATE";
	}

	query << " ON " << db << ".* TO " << user << "@'%'";
	const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
	stmt->execute(query.str());
}

void MySQLQueryExecutor::execute(const std::string& query) {
	const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
	stmt->execute(query);
}

DatabaseDetails MySQLQueryExecutor::details() {
	return details_;
}

std::vector<Migration> MySQLQueryExecutor::migrations() {
	std::vector<Migration> migrations;

	std::string query("SELECT `id`, `core_version`, `commit`, `install_date`, `installed_by`, "
	                  "`file` FROM `schema_history` ORDER BY `id` ASC");
	const auto stmt = std::unique_ptr<sql::Statement>(conn_->createStatement());
	auto results = std::unique_ptr<sql::ResultSet>(stmt->executeQuery(std::move(query)));

	while(results->next()) {
		Migration migration {
			.id = results->getUInt("id"),
			.core_version = results->getString("core_version"),
			.install_date = results->getString("install_date"),
			.installed_by = results->getString("installed_by"),
			.commit_hash = results->getString("commit"),
			.file = results->getString("file")
		};

		migrations.emplace_back(std::move(migration));
	}
	
	return migrations;
}

void MySQLQueryExecutor::select_db(const std::string& schema) {
	conn_->setSchema(schema);
}

void MySQLQueryExecutor::start_transaction() {
	conn_->setAutoCommit(false);
}

void MySQLQueryExecutor::end_transaction() {
	conn_->commit();
	conn_->setAutoCommit(true);
}

void MySQLQueryExecutor::rollback() {
	conn_->rollback();
}

void MySQLQueryExecutor::insert_migration_meta(const Migration& meta) {
	const auto query = "INSERT INTO `schema_history` "
	                    "(`core_version`, `installed_by`, `install_date`, `commit`, `file`) "
	                    "VALUES "
	                    "(?, ?, UTC_TIMESTAMP(), ?, ?)";

	const auto stmt = std::unique_ptr<sql::PreparedStatement>(conn_->prepareStatement(query));
	stmt->setString(1, meta.core_version);
	stmt->setString(2, meta.installed_by);
	stmt->setString(3, meta.commit_hash);
	stmt->setString(4, meta.file);
	stmt->execute();
}