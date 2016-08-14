/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <dbcreader/Storage.h>
#include <game_protocol/ResultCodes.h>
#include <shared/database/daos/CharacterDAO.h>
#include <spark/temp/Character_generated.h>
#include <shared/util/PCREHelper.h>
#include <logger/Logging.h>
//#include <boost/locale.hpp>
#include <boost/optional.hpp>
#include <pcre.h>
#include <locale>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember {

class ThreadPool;

class CharacterHandler {
	typedef std::function<void(protocol::ResultCode)> CharacterCreateCB;
	typedef std::function<void(boost::optional<std::vector<Character>>)> CharacterEnumCB;
	typedef std::function<void(protocol::ResultCode)> CharacterDeleteCB;

	// todo, should probably be in a config
	const std::size_t MAX_NAME_LENGTH = 12;
	const std::size_t MIN_NAME_LENGTH = 2;
	const std::size_t MAX_CONSECUTIVE_LETTERS = 2;
	const std::size_t MAX_CHARACTER_SLOTS = 10;

	const std::vector<util::pcre::Result>& profane_names_;
	const std::vector<util::pcre::Result>& reserved_names_;
	const dbc::Storage& dbc_;
	const dal::CharacterDAO& dao_;
	const std::locale locale_;

	ThreadPool& pool_;
	log::Logger* logger_;

	void validate_race();
	void validate_class();
	void validate_race_class_pair();
	protocol::ResultCode validate_name(const std::string& name) const;
	bool validate_options(const messaging::character::Character& character, std::uint32_t account_id) const;

	void on_enum_complete(boost::optional<std::vector<Character>> characters,
	                      Character& character, CharacterCreateCB cb) const;


	void name_collision_callback(const std::string& name, std::uint32_t realm_id, CharacterCreateCB cb) const;

public:
	CharacterHandler(const std::vector<util::pcre::Result>& profane_names,
	                 const std::vector<util::pcre::Result>& reserved_names,
	                 const dbc::Storage& dbc, const dal::CharacterDAO& dao,
	                 ThreadPool& pool, const std::locale& locale, log::Logger* logger);

	void create_character(std::uint32_t account_id, std::uint32_t realm_id,
	                      const messaging::character::CharacterTemplate& character,
	                      CharacterCreateCB cb) const;

	void delete_character(std::uint32_t account_id, std::uint32_t realm_id, std::uint64_t character_guid,
	                      CharacterDeleteCB cb) const;

	void enum_characters(std::uint32_t account_id, std::uint32_t realm_id, CharacterEnumCB cb) const;

	void rename_character(std::uint32_t account_id, std::uint64_t character_guid,
	                      const std::string& name, CharacterCreateCB cb);
};

} // ember