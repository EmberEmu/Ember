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
#include <boost/algorithm/string.hpp>
#include <pcre.h>

namespace ember {

CharacterHandler::CharacterHandler(const dbc::Storage& dbc, const dal::CharacterDAO& dao, const std::string& locale)
                                   : dbc_(dbc), dao_(dao) {

	//boost::locale::generator gen_;
	//locale_ = gen_.generate(locale);

	for(auto& i : dbc_.names_profanity.values()) {
		int error_offset = 0;
		const char* error = 0;

		std::string target = i.name; // because we're working with const data - move?
		boost::replace_first(target, R"(\<)", R"(\b)");
		boost::replace_first(target, R"(\>)", R"(\b)");
		
		auto expression = std::unique_ptr<pcre, void(*)(void*)>(pcre_compile(target.c_str(), PCRE_UTF8 | PCRE_CASELESS,
		                                                     &error, &error_offset, nullptr), pcre_free);

		if(expression == nullptr) {
			LOG_DEBUG_GLOB << "Nope" << LOG_ASYNC;
		}

		auto extra = std::unique_ptr<pcre_extra, void(*)(pcre_extra*)>(pcre_study(expression.get(), PCRE_STUDY_JIT_COMPILE,
		                                                                    &error), pcre_free_study);

		if(extra == nullptr) {
			LOG_DEBUG_GLOB << "Nope #2" << LOG_ASYNC;
		}

		/*int ignored;
		auto matches = pcre_exec(expression.get(), nullptr, find.c_str(), find.size(), 0, 0, &ignored, 1);

		if(matches >= 0) {
			LOG_DEBUG_GLOB << "Match found with " << target << LOG_ASYNC;
		} else {
			if(matches != PCRE_ERROR_NOMATCH) {
				LOG_DEBUG_GLOB << "Error with " << target << LOG_ASYNC;
			}
		}*/

		// workaround broken grep grammar implementation by removing the \< & \> word boundaries
		//std::string pattern = std::regex_replace(test, boundary_replace, L"");

		//std::regex regex(i.name);
		//regex_profane_.emplace_back(regex);
	}

	/*for(auto& i : dbc_.names_reserved.values()) {
		std::cout << i.name;
		// workaround broken grep grammar implementation by removing the \< & \> word boundaries
		std::string pattern = std::regex_replace(i.name, boundary_replace, "");
		//regex_reserved_.emplace_back(pattern, std::regex::grep | std::regex::icase | std::regex::optimize);
	}*/
}

protocol::ResultCode CharacterHandler::validate_name(const std::string& name) const {
	if(name.empty()) {
		return protocol::ResultCode::CHAR_NAME_NO_NAME;
	}

	// we treat std::string as a container for a UTF-8 encoded string
	bool valid = false;
	std::size_t name_length = util::utf8::length(name, valid);

	if(!valid) {
		return protocol::ResultCode::CHAR_NAME_FAILURE;
	}

	if(name_length == 0) {
		return protocol::ResultCode::CHAR_NAME_NO_NAME;
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