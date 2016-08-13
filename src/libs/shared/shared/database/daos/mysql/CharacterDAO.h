/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/CharacterBase.h>
#include <conpool/ConnectionPool.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <cppconn/prepared_statement.h>
#include <chrono>
#include <memory>
#include <vector>

namespace ember { namespace dal { 

using namespace std::chrono_literals;

/* TODO, TEMPORARY CODE*/

template<typename T>
class MySQLCharacterDAO final : public CharacterDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLCharacterDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	boost::optional<Character> character(const std::string& name, std::uint32_t realm_id) const override try {
		const std::string query = "SELECT c.name, c.id, c.account_id, c.realm_id, c.race, c.class, c.gender, "
		                          "c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
		                          "c.map, c.x, c.y, c.z, c.flags, c.first_login, c.pet_display, c.pet_level, "
		                          "c.pet_family, gc.id as guild_id "
		                          "FROM characters c "
		                          "LEFT JOIN guild_characters gc ON c.id = gc.character_id "
		                          "WHERE name = ? AND realm_id = ?";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, name);
		stmt->setUInt(2, realm_id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			Character character(res->getString("name"), res->getUInt("id"), res->getUInt("account_id"),
			                    res->getUInt("realm_id"), res->getUInt("race"),
			                    res->getUInt("class"), res->getUInt("gender"), res->getUInt("skin"),
			                    res->getUInt("face"), res->getUInt("hairstyle"), res->getUInt("haircolour"),
			                    res->getUInt("facialhair"), res->getUInt("level"), res->getUInt("zone"),
			                    res->getUInt("map"), res->getUInt("guild_id"), res->getDouble("x"), res->getDouble("y"),
			                    res->getDouble("z"), res->getUInt("flags"), res->getUInt("first_login"),
			                    res->getUInt("pet_display"), res->getUInt("pet_level"), res->getUInt("pet_family"));
			return character;
		}

		return boost::optional<Character>();
	} catch(std::exception& e) {
		throw exception(e.what());
	}
	
	boost::optional<Character> character(unsigned int id) const override try {
		const std::string query = "SELECT c.name, c.id, c.account_id, c.realm_id, c.race, c.class, c.gender, "
		                          "c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
		                          "c.map, c.x, c.y, c.z, c.flags, c.first_login, c.pet_display, c.pet_level, "
		                          "c.pet_family, gc.id as guild_id "
		                          "FROM characters c "
		                          "LEFT JOIN guild_characters gc ON c.id = gc.character_id "
		                          "WHERE id = ?";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt(1, id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			Character character(res->getString("name"), res->getUInt("id"), res->getUInt("account_id"),
			                    res->getUInt("realm_id"), res->getUInt("race"),
			                    res->getUInt("class"), res->getUInt("gender"), res->getUInt("skin"),
			                    res->getUInt("face"), res->getUInt("hairstyle"), res->getUInt("haircolour"),
			                    res->getUInt("facialhair"), res->getUInt("level"), res->getUInt("zone"),
			                    res->getUInt("map"), res->getUInt("guild_id"), res->getDouble("x"), res->getDouble("y"),
			                    res->getDouble("z"), res->getUInt("flags"), res->getUInt("first_login"),
			                    res->getUInt("pet_display"), res->getUInt("pet_level"), res->getUInt("pet_family"));
			return character;
		}

		return boost::optional<Character>();
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	std::vector<Character> characters(std::uint32_t account_id, std::uint32_t realm_id) const override try {
		const std::string query = "SELECT c.name, c.id, c.account_id, c.realm_id, c.race, c.class, c.gender, "
		                          "c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
		                          "c.map, c.x, c.y, c.z, c.flags, c.first_login, c.pet_display, c.pet_level, "
		                          "c.pet_family, gc.id as guild_id "
		                          "FROM characters c "
		                          "LEFT JOIN guild_characters gc ON c.id = gc.character_id "
		                          "LEFT JOIN users u ON u.id = c.account_id "
		                          "WHERE u.id = ? AND c.realm_id = ?";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt(1, account_id);
		stmt->setUInt(2, realm_id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<Character> characters;

		while(res->next()) {
			Character character(res->getString("name"), res->getUInt("id"), res->getUInt("account_id"),
			                    res->getUInt("realm_id"), res->getUInt("race"),
			                    res->getUInt("class"), res->getUInt("gender"), res->getUInt("skin"),
			                    res->getUInt("face"), res->getUInt("hairstyle"), res->getUInt("haircolour"),
			                    res->getUInt("facialhair"), res->getUInt("level"), res->getUInt("zone"),
			                    res->getUInt("map"), res->getUInt("guild_id"), res->getDouble("x"), res->getDouble("y"),
			                    res->getDouble("z"), res->getUInt("flags"), res->getUInt("first_login"),
			                    res->getUInt("pet_display"), res->getUInt("pet_level"), res->getUInt("pet_family"));
			characters.emplace_back(character);
		}

		return characters;
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	void delete_character(unsigned int id) const override try {
		const std::string query = "DELETE FROM characters WHERE id = ?";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setInt(1, id);
		
		if(!stmt->executeUpdate()) {
			throw exception("Unable to delete character " + std::to_string(id));
		}
	} catch(std::exception& e) {
		throw exception(e.what());
	}

	void create(const Character& character) const override try {
		const std::string query = "INSERT INTO characters (name, account_id, realm_id, race, class, gender, "
		                          "skin, face, hairstyle, haircolour, facialhair, level, zone, "
		                          "map, x, y, z, flags, first_login, pet_display, pet_level, "
		                          "pet_family) "
		                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
		
		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, character.name());
		stmt->setUInt(2, character.account_id());
		stmt->setUInt(3, character.realm_id());
		stmt->setUInt(4, character.race());
		stmt->setUInt(5, character.class_temp());
		stmt->setUInt(6, character.gender());
		stmt->setUInt(7, character.skin());
		stmt->setUInt(8, character.face());
		stmt->setUInt(9, character.hairstyle());
		stmt->setUInt(10, character.haircolour());
		stmt->setUInt(11, character.facialhair());
		stmt->setUInt(12, character.level());
		stmt->setUInt(13, character.zone());
		stmt->setUInt(14, character.map());
		stmt->setDouble(15, character.x());
		stmt->setDouble(16, character.y());
		stmt->setDouble(17, character.z());
		stmt->setUInt(18, character.flags());
		stmt->setUInt(19, character.first_login());
		stmt->setUInt(20, character.pet_display());
		stmt->setUInt(21, character.pet_level());
		stmt->setUInt(22, character.pet_family());

		if(!stmt->executeUpdate()) {
			throw exception("Unable to create character");
		}
	} catch(std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
std::unique_ptr<MySQLCharacterDAO<T>> character_dao(T& pool) {
	return std::make_unique<MySQLCharacterDAO<T>>(pool);
}

}} //dal, ember