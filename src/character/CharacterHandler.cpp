/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterHandler.h"
#include <shared/util/Utility.h>
#include <cctype>
#include <boost/locale.hpp>

namespace ember {

CharacterHandler::CharacterHandler(const dbc::Storage& dbc, const dal::CharacterDAO& dao)
                                   : dbc_(dbc), dao_(dao) {
	std::wregex boundary_replace(LR"(\<|\>)");
	boost::locale::generator gen;
	std::locale loc = gen("en_GB.UTF-8");
	std::locale::global(loc);
	for(auto& i : dbc_.names_profanity.values()) {
		
		// workaround broken grep grammar implementation by removing the \< & \> word boundaries
		//std::string pattern = std::regex_replace(test, boundary_replace, L"");

		std::regex regex(i.name);
		//regex_profane_.emplace_back(regex);
	}

	/*for(auto& i : dbc_.names_reserved.values()) {
		std::cout << i.name;
		// workaround broken grep grammar implementation by removing the \< & \> word boundaries
		std::string pattern = std::regex_replace(i.name, boundary_replace, "");
		//regex_reserved_.emplace_back(pattern, std::regex::grep | std::regex::icase | std::regex::optimize);
	}*/
}

protocol::ResultCode CharacterHandler::validate_name(const std::string& name) {
	if(name.empty()) {
		return protocol::ResultCode::CHAR_NAME_NO_NAME;
	}

	if(name.size() > MAX_NAME_LENGTH) {
		return protocol::ResultCode::CHAR_NAME_TOO_LONG;
	}

	if(name.size() < MIN_NAME_LENGTH) {
		return protocol::ResultCode::CHAR_NAME_TOO_SHORT;
	}

	if(util::max_consecutive_check(name) > MAX_CONSECUTIVE_LETTERS) {
		return protocol::ResultCode::CHAR_NAME_THREE_CONSECUTIVE;
	}

	bool alpha_only = std::find_if(name.begin(), name.end(), [](char c) {
		return std::isalpha(c);
	}) != name.end();

	if(!alpha_only) {
		return protocol::ResultCode::CHAR_NAME_ONLY_LETTERS;
	}

	for(auto& regex : regex_profane_) {
		/*if(std::wregex_match(name, regex)) {
			return protocol::ResultCode::CHAR_NAME_PROFANE;
		}*/
	}

	for(auto& regex : regex_reserved_) {
		/*if(std::wregex_match(name, regex)) {
			return protocol::ResultCode::CHAR_NAME_RESERVED;
		}*/
	}

	return protocol::ResultCode::CHAR_CREATE_SUCCESS;

}

void CharacterHandler::create_character(std::uint32_t account_id, std::uint32_t realm_id,
                                        const messaging::character::Character& details) const {
	
}

void CharacterHandler::delete_character(std::uint32_t account_id, std::uint32_t realm_id,
                                        std::uint64_t character_guid) const {

}

std::vector<int> CharacterHandler::enum_characters(std::uint32_t account_id, std::uint32_t realm_id) const {
	return std::vector<int>();
}

} // ember