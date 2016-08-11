/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterHandler.h"
#include <shared/util/Utility.h>
#include <shared/util/UTF8.h>
#include <utf8cpp/utf8.h>

namespace ember {

CharacterHandler::CharacterHandler(const std::vector<util::pcre::Result>& profane_names,
                                   const std::vector<util::pcre::Result>& reserved_names,
                                   const dbc::Storage& dbc, const dal::CharacterDAO& dao,
                                   const std::locale& locale)
                                   : profane_names_(profane_names), reserved_names_(reserved_names),
                                     dbc_(dbc), dao_(dao), locale_(locale) { }

protocol::ResultCode CharacterHandler::validate_name(const std::string& name) const {
	if(name.empty()) {
		return protocol::ResultCode::CHAR_NAME_NO_NAME;
	}

	bool valid = false;
	std::size_t name_length = util::utf8::length(name, valid);

	if(!valid) { // wasn't a valid UTF-8 encoded string
		return protocol::ResultCode::CHAR_NAME_FAILURE;
	}

	if(name_length > MAX_NAME_LENGTH) {
		return protocol::ResultCode::CHAR_NAME_TOO_LONG;
	}

	if(name_length < MIN_NAME_LENGTH) {
		return protocol::ResultCode::CHAR_NAME_TOO_SHORT;
	}

	if(util::max_consecutive_check(name) > MAX_CONSECUTIVE_LETTERS) { // todo, not unicode aware
		return protocol::ResultCode::CHAR_NAME_THREE_CONSECUTIVE;
	}

	// this is probably all horribly wrong
	//std::vector<wchar_t> utf16_name;
	//utf8::unchecked::utf8to16(name.begin(), name.end(), std::back_inserter(utf16_name));

	//bool alpha_only = std::find_if(utf16_name.begin(), utf16_name.end(), [&](wchar_t c) {
	//	return !std::iswalpha(c);
	//}) != utf16_name.end();

	//if(!alpha_only) {
	//	return protocol::ResultCode::CHAR_NAME_ONLY_LETTERS;
	//}

	for(auto& regex : reserved_names_) {
		int ret = util::pcre::match(name, regex);
			
		if(ret >= 0) {
			return protocol::ResultCode::CHAR_NAME_RESERVED;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_WARN_GLOB << "PCRE error encountered: " + ret << LOG_ASYNC;
			return protocol::ResultCode::CHAR_NAME_FAILURE;
		}
	}

	for(auto& regex : profane_names_) {
		int ret = util::pcre::match(name, regex);

		if(ret >= 0) {
			return protocol::ResultCode::CHAR_NAME_PROFANE;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_WARN_GLOB << "PCRE error encountered: " + ret << LOG_ASYNC;
			return protocol::ResultCode::CHAR_NAME_FAILURE;
		}
	}

	return protocol::ResultCode::CHAR_CREATE_SUCCESS;

}

std::string CharacterHandler::create_character(std::uint32_t account_id, std::uint32_t realm_id,
                                                        const messaging::character::Character& details) const {
	std::string name = details.name()->c_str();

	//try {
	//	// convert name case
	//	name = boost::locale::to_title(name, locale_);
	//} catch(std::bad_cast& e) {
	//	LOG_DEBUG_GLOB << "Could not perform name-case conversion: " << e.what() << LOG_ASYNC;
	//	return "";
	//}

	auto success = validate_name(name);
	LOG_DEBUG_GLOB << protocol::to_string(success) << LOG_ASYNC;
	LOG_DEBUG_GLOB << name << LOG_ASYNC;
	return name;
}

void CharacterHandler::delete_character(std::uint32_t account_id, std::uint32_t realm_id,
                                        std::uint64_t character_guid) const {

}

std::vector<int> CharacterHandler::enum_characters(std::uint32_t account_id, std::uint32_t realm_id) const {
	return std::vector<int>();
}

} // ember