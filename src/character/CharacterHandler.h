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
#include <logger/Logging.h>
#include <boost/locale.hpp>
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

	std::locale locale_;
	std::vector<std::regex> regex_profane_;
	std::vector<std::regex> regex_reserved_;

	const dbc::Storage& dbc_;
	const dal::CharacterDAO& dao_;

	protocol::ResultCode validate_name(const std::string& name) const;
	void validate_race();
	void validate_class();
	void validate_race_class_pair();

public:
	CharacterHandler(const dbc::Storage& dbc, const dal::CharacterDAO& dao, const std::string& locale);

	std::string create_character(std::uint32_t account_id, std::uint32_t realm_id,
	                                      const messaging::character::Character& details) const;
	void delete_character(std::uint32_t account_id, std::uint32_t realm_id, std::uint64_t character_guid) const;
	std::vector<int> enum_characters(std::uint32_t account_id, std::uint32_t realm_id) const;
};

} // ember