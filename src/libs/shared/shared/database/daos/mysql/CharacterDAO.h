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

namespace ember::dal { 

using namespace std::chrono_literals;

template<typename T>
class MySQLCharacterDAO final : public CharacterDAO {
	T& pool_;
	drivers::MySQL* driver_;

	Character result_to_character(sql::ResultSet* res) const {
		Character character;
		character.name = res->getString("name");
		character.internal_name = res->getString("internal_name");
		character.id = res->getUInt("id");
		character.account_id = res->getUInt("account_id");
		character.realm_id = res->getUInt("realm_id");
		character.race = res->getUInt("race");
		character.class_ = res->getUInt("class");
		character.gender = res->getUInt("gender");
		character.skin = res->getUInt("skin");
		character.face = res->getUInt("face");
		character.hairstyle = res->getUInt("hairstyle");
		character.haircolour = res->getUInt("haircolour");
		character.facialhair = res->getUInt("facialhair");
		character.level = res->getUInt("level");
		character.zone = res->getUInt("zone");
		character.map = res->getUInt("map");
		character.guild_id = res->getUInt("guild_id");
		character.guild_rank = res->getUInt("guild_rank");
		character.position.x = res->getDouble("x");
		character.position.y = res->getDouble("y");
		character.position.z = res->getDouble("z");
		character.orientation = res->getDouble("o");
		character.flags = static_cast<Character::Flags>(res->getUInt("flags"));
		character.first_login = res->getBoolean("first_login");
		character.pet_display = res->getUInt("pet_display");
		character.pet_level = res->getUInt("pet_level");
		character.pet_family = res->getUInt("pet_family");
		return character;
	}

public:
	MySQLCharacterDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::optional<Character> character(const std::string& name, std::uint32_t realm_id) const override try {
		const std::string query = "SELECT c.name, c.internal_name, c.id, c.account_id, c.realm_id, c.race, c.class, "
		                          "c.gender, c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
		                          "c.map, c.x, c.y, c.z, c.o, c.flags, c.first_login, c.pet_display, c.pet_level, "
		                          "c.pet_family, gc.id as guild_id, gc.rank as guild_rank "
		                          "FROM characters c "
		                          "LEFT JOIN guild_characters gc ON c.id = gc.character_id "
		                          "WHERE internal_name = ? AND realm_id = ? AND c.deletion_date IS NULL";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, name);
		stmt->setUInt(2, realm_id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			return result_to_character(res.get());
		}

		return std::nullopt;
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
	
	std::optional<Character> character(std::uint64_t id) const override try {
		const std::string query = "SELECT c.name, c.internal_name, c.id, c.account_id, c.realm_id, c.race, c.class, "
		                          "c.gender, c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
		                          "c.map, c.x, c.y, c.z, c.o, c.flags, c.first_login, c.pet_display, c.pet_level, "
		                          "c.pet_family, gc.id as guild_id, gc.rank as guild_rank "
		                          "FROM characters c "
		                          "LEFT JOIN guild_characters gc ON c.id = gc.character_id "
		                          "WHERE c.id = ? AND c.deletion_date IS NULL";

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt64(1, id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

		if(res->next()) {
			return result_to_character(res.get());;
		}

		return std::nullopt;
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::vector<Character> characters(std::uint32_t account_id, std::uint32_t realm_id = 0) const override try {
		std::string query = "SELECT c.name, c.internal_name, c.id, c.account_id, c.realm_id, c.race, c.class, "
		                    "c.gender, c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
		                    "c.map, c.x, c.y, c.z, c.o, c.flags, c.first_login, c.pet_display, c.pet_level, "
		                    "c.pet_family, gc.id as guild_id, gc.rank as guild_rank "
		                    "FROM characters c "
		                    "LEFT JOIN guild_characters gc ON c.id = gc.character_id "
		                    "LEFT JOIN users u ON u.id = c.account_id "
		                    "WHERE u.id = ? AND c.deletion_date IS NULL ";

		if(realm_id) {
			query.append("AND c.realm_id = ? ");
		}

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt(1, account_id);

		if(realm_id) {
			stmt->setUInt(2, realm_id);
		}

		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<Character> characters;

		while(res->next()) {
			characters.emplace_back(result_to_character(res.get()));
		}

		return characters;
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void restore(std::uint64_t id) const override try {
		const std::string query = "UPDATE characters SET deletion_date = NULL WHERE id = ?";
	

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt64(1, id);

		if(!stmt->executeUpdate()) {
			throw exception("Unable to restore character " + std::to_string(id));
		}
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void delete_character(std::uint64_t id, bool soft_delete) const override try {
		std::string query;

		if(soft_delete) {
			query = "UPDATE characters SET deletion_date = CURTIME(), internal_name = CONCAT(name, id) WHERE id = ?";
		} else {
			query = "DELETE FROM characters WHERE id = ?";
		}

		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setUInt64(1, id);
		
		if(!stmt->executeUpdate()) {
			throw exception("Unable to delete character " + std::to_string(id));
		}
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void create(const Character& character) const override try {
		const std::string query = "INSERT INTO characters (name, account_id, realm_id, race, class, gender, "
		                          "skin, face, hairstyle, haircolour, facialhair, level, zone, "
		                          "map, x, y, z, o, flags, first_login, pet_display, pet_level, "
		                          "pet_family, internal_name) "
		                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
		
		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, character.name);
		stmt->setUInt(2, character.account_id);
		stmt->setUInt(3, character.realm_id);
		stmt->setUInt(4, character.race);
		stmt->setUInt(5, character.class_);
		stmt->setUInt(6, character.gender);
		stmt->setUInt(7, character.skin);
		stmt->setUInt(8, character.face);
		stmt->setUInt(9, character.hairstyle);
		stmt->setUInt(10, character.haircolour);
		stmt->setUInt(11, character.facialhair);
		stmt->setUInt(12, character.level);
		stmt->setUInt(13, character.zone);
		stmt->setUInt(14, character.map);
		stmt->setDouble(15, character.position.x);
		stmt->setDouble(16, character.position.y);
		stmt->setDouble(17, character.position.z);
		stmt->SetDouble(18, character.orientation);
		stmt->setUInt(19, static_cast<std::uint32_t>(character.flags));
		stmt->setUInt(20, character.first_login);
		stmt->setUInt(21, character.pet_display);
		stmt->setUInt(22, character.pet_level);
		stmt->setUInt(23, character.pet_family);
		stmt->setString(24, character.internal_name);

		if(!stmt->executeUpdate()) {
			throw exception("Unable to create character");
		}
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void update(const Character& character) const override try {
		const std::string query = "UPDATE characters SET name = ?, internal_name = ?, account_id = ?, "
		                          "realm_id = ?, race = ?, class = ?, gender = ?, skin = ?, face = ?, "
		                          "hairstyle = ?, haircolour = ?, facialhair = ?, level = ?, zone = ?, "
		                          "map = ?, x = ?, y = ?, z = ?, o = ?, flags = ?, first_login = ?, pet_display = ?, "
		                          "pet_level = ?, pet_family = ? "
		                          "WHERE id = ?";
		
		auto conn = pool_.wait_connection(5s);
		sql::PreparedStatement* stmt = driver_->prepare_cached(*conn, query);
		stmt->setString(1, character.name);
		stmt->setString(2, character.internal_name);
		stmt->setUInt(3, character.account_id);
		stmt->setUInt(4, character.realm_id);
		stmt->setUInt(5, character.race);
		stmt->setUInt(6, character.class_);
		stmt->setUInt(7, character.gender);
		stmt->setUInt(8, character.skin);
		stmt->setUInt(9, character.face);
		stmt->setUInt(10, character.hairstyle);
		stmt->setUInt(11, character.haircolour);
		stmt->setUInt(12, character.facialhair);
		stmt->setUInt(13, character.level);
		stmt->setUInt(14, character.zone);
		stmt->setUInt(15, character.map);
		stmt->setDouble(16, character.position.x);
		stmt->setDouble(17, character.position.y);
		stmt->setDouble(18, character.position.z);
		stmt->setDouble(19, character.orientation);
		stmt->setUInt(20, static_cast<std::uint32_t>(character.flags));
		stmt->setUInt(21, character.first_login);
		stmt->setUInt(22, character.pet_display);
		stmt->setUInt(23, character.pet_level);
		stmt->setUInt(24, character.pet_family);
		stmt->setUInt64(25, character.id);

		if(!stmt->executeUpdate()) {
			throw exception("Unable to update character");
		}
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
std::unique_ptr<MySQLCharacterDAO<T>> character_dao(T& pool) {
	return std::make_unique<MySQLCharacterDAO<T>>(pool);
}

} //dal, ember