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
#include <pcre.h>
#include <locale>
#include <regex>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember {

class CharacterHandler {
	const std::size_t MAX_NAME_LENGTH = 12;
	const std::size_t MIN_NAME_LENGTH = 2;
	const std::size_t MAX_CONSECUTIVE_LETTERS = 2;

	const std::vector<util::pcre::Result>& profane_names_;
	const std::vector<util::pcre::Result>& reserved_names_;
	const dbc::Storage& dbc_;
	const dal::CharacterDAO& dao_;
	const std::locale locale_;
	log::Logger* logger_;

	void validate_race();
	void validate_class();
	void validate_race_class_pair();
	protocol::ResultCode validate_name(const std::string& name) const;
	bool validate_options(const messaging::character::Character& character, std::uint32_t account_id) const;

public:
	CharacterHandler(const std::vector<util::pcre::Result>& profane_names,
	                 const std::vector<util::pcre::Result>& reserved_names,
	                 const dbc::Storage& dbc, const dal::CharacterDAO& dao,
	                 const std::locale& locale, log::Logger* logger);

	protocol::ResultCode create_character(std::uint32_t account_id, std::uint32_t realm_id,
	                                      const messaging::character::Character& character) const;
	void delete_character(std::uint32_t account_id, std::uint32_t realm_id, std::uint64_t character_guid) const;
	std::vector<int> enum_characters(std::uint32_t account_id, std::uint32_t realm_id) const;
};

} // ember