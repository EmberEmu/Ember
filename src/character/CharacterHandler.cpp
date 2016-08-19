/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterHandler.h"
#include "FilterTypes.h"
#include <shared/util/Utility.h>
#include <shared/util/UTF8.h>
#include <shared/threading/ThreadPool.h>
#include <utf8cpp/utf8.h>

namespace ember {

CharacterHandler::CharacterHandler(const std::vector<util::pcre::Result>& profane_names,
                                   const std::vector<util::pcre::Result>& reserved_names,
                                   const dbc::Storage& dbc, const dal::CharacterDAO& dao,
                                   ThreadPool& pool, const std::locale& locale, log::Logger* logger)
                                   : profane_names_(profane_names), reserved_names_(reserved_names),
                                     dbc_(dbc), dao_(dao), pool_(pool), locale_(locale), logger_(logger) { }

void CharacterHandler::create_character(std::uint32_t account_id, std::uint32_t realm_id,
                                        const messaging::character::CharacterTemplate& options,
                                        ResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	Character character{};
	character.race = options.race();
	character.name = options.name()->c_str();
	character.account_id = account_id;
	character.realm_id = realm_id;
	character.class_ = options.class_();
	character.gender = options.gender();
	character.skin = options.skin();
	character.face = options.face();
	character.hairstyle = options.hairstyle();
	character.haircolour = options.haircolour();
	character.facialhair = options.facialhair();
	character.level = 1; // todo
	character.flags = Character::Flags::NONE; // todo
	character.first_login = true;

	// class, race and visual customisation validation
	bool success = validate_options(character, account_id);

	if(!success) {
		callback(protocol::ResultCode::CHAR_CREATE_ERROR);
		return;
	}
	
	// name validation
	auto result = validate_name(character.name);

	if(result != protocol::ResultCode::CHAR_NAME_SUCCESS) {
		callback(result);
		return;
	}

	// query database for further validation steps
	name_collision_callback(character.name, realm_id, [=](protocol::ResultCode result) mutable {
		if(result == protocol::ResultCode::CHAR_NAME_SUCCESS) {
			enum_characters(account_id, realm_id, [=](auto& characters) mutable {
				on_enum_complete(characters, character, callback);
			});
		} else {
			callback(result);
		}
	});
}

void CharacterHandler::delete_callback(std::uint32_t account_id, std::uint32_t realm_id,
                                       std::uint64_t character_id, const boost::optional<Character>& character,
                                       const ResultCB& callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	// character must exist, belong to the same account and be on the same realm
	if(!character || character->account_id != account_id || character->realm_id != realm_id) {
		LOG_WARN_FILTER(logger_, LF_NAUGHTY_USER)
			<< "Account " << account_id << " attempted an invalid delete on character "
			<< character_id << LOG_ASYNC;
		callback(protocol::ResultCode::CHAR_DELETE_FAILED);
		return;
	}

	if((character->flags & Character::Flags::LOCKED_FOR_TRANSFER) == Character::Flags::LOCKED_FOR_TRANSFER) {
		callback(protocol::ResultCode::CHAR_DELETE_FAILED_LOCKED_FOR_TRANSFER);
		return;
	}

	// character cannot be a guild leader (no specific guild leader deletion message until TBC)
	if(character->guild_rank == 1) { // todo, ranks need defined properly
		callback(protocol::ResultCode::CHAR_DELETE_FAILED);
		return;
	}

	pool_.run([=] {
		try {
			dao_.delete_character(character_id, true);
			callback(protocol::ResultCode::CHAR_DELETE_SUCCESS);
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::CHAR_DELETE_FAILED);
		}
	});
}

void CharacterHandler::restore_character(std::uint64_t id, ResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		try {
			auto character = dao_.character(id);
			auto match = dao_.character(character->name, character->realm_id);
			auto char_enum = dao_.characters(character->account_id, character->realm_id);

			if(char_enum.size() >= MAX_CHARACTER_SLOTS) {
				LOG_WARN(logger_) << "Cannot restore character - would exceed max slots" << LOG_ASYNC;
				callback(protocol::ResultCode::RESPONSE_FAILURE);
				return;
			}

			if(match) {
				// if the character name has been reused, set the rename flag
				// and don't restore their internal name
				character->flags |= Character::Flags::RENAME;
				dao_.update(*character, false);
				dao_.restore(id, false);
			} else {
				dao_.restore(id, true);
			}

			callback(protocol::ResultCode::RESPONSE_SUCCESS);
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::RESPONSE_FAILURE);
		}
	});
}

void CharacterHandler::delete_character(std::uint32_t account_id, std::uint32_t realm_id,
                                        std::uint64_t character_id, ResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		try {
			auto character = dao_.character(character_id);
			delete_callback(account_id, realm_id, character_id, character, callback);
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::CHAR_DELETE_FAILED);
		}
	});
}

void CharacterHandler::enum_characters(std::uint32_t account_id, std::uint32_t realm_id,
                                       EnumResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		try {
			auto characters = dao_.characters(account_id, realm_id);
			callback(std::move(characters));
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(boost::optional<std::vector<Character>>());
		}
	});
}

void CharacterHandler::rename_finalise(Character character, const std::string& name,
                                       const RenameCB& callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	character.name = name;
	character.flags ^= Character::Flags::RENAME;

	pool_.run([=] {
		try {
			dao_.update(character, true);
			callback(protocol::ResultCode::RESPONSE_SUCCESS, character);
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::CHAR_NAME_FAILURE, {});
		}
	});
}

void CharacterHandler::rename_validate(std::uint32_t account_id,
                                       const boost::optional<Character>& character,
                                       const std::string& name, const RenameCB& callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!character) {
		callback(protocol::ResultCode::CHAR_NAME_FAILURE, {});
		return;
	}

	if(character->account_id != account_id) {
		callback(protocol::ResultCode::CHAR_NAME_FAILURE, {});
		return;
	}

	if((character->flags & Character::Flags::RENAME) != Character::Flags::RENAME) {
		callback(protocol::ResultCode::CHAR_NAME_FAILURE, {});
		return;
	}

	auto res = validate_name(name);

	if(res != protocol::ResultCode::CHAR_NAME_SUCCESS) {
		callback(res, {});
		return;
	}

	name_collision_callback(name, character->realm_id, [=](protocol::ResultCode result) mutable {
		if(result == protocol::ResultCode::CHAR_NAME_SUCCESS) {
			rename_finalise(*character, name, callback); // todo, better names
		} else {
			callback(result, {});
		}
	});
}

void CharacterHandler::rename_character(std::uint32_t account_id, std::uint64_t character_id,
                                        const std::string& name, RenameCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		try {
			auto character = dao_.character(character_id);
			rename_validate(account_id, character, name, callback);
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::CHAR_NAME_FAILURE, {});
		}
	});

}

protocol::ResultCode CharacterHandler::validate_name(const std::string& name) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

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
			LOG_ERROR(logger_) << "PCRE error encountered: " + ret << LOG_ASYNC;
			return protocol::ResultCode::CHAR_NAME_FAILURE;
		}
	}

	for(auto& regex : profane_names_) {
		int ret = util::pcre::match(name, regex);

		if(ret >= 0) {
			return protocol::ResultCode::CHAR_NAME_PROFANE;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_) << "PCRE error encountered: " + ret << LOG_ASYNC;
			return protocol::ResultCode::CHAR_NAME_FAILURE;
		}
	}

	return protocol::ResultCode::CHAR_NAME_SUCCESS;
}

bool CharacterHandler::validate_options(const Character& character, std::uint32_t account_id) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	// validate the race/class combination
	auto found = std::find_if(dbc_.char_base_info.begin(), dbc_.char_base_info.end(), [&](auto val) {
		return (character.class_ == val.second.class__id && character.race == val.second.race_id);
	});

	if(found == dbc_.char_base_info.end()) {
		LOG_WARN_FILTER(logger_, LF_NAUGHTY_USER)
			<< "Invalid class/race combination of " << character.class_
			<< " & " << character.race << " from account ID " << account_id << LOG_ASYNC;
		return false;
	}

	bool skin_match = false;
	bool hair_match = false;
	bool face_match = false;

	// validate visual customisation options
	for(auto& section : dbc_.char_sections.values()) {
		if(section.npc_only || section.race_id != character.race
		   || section.sex != static_cast<dbc::CharSections::Sex>(character.gender)) {
			continue;
		}

		switch(section.type) {
			case dbc::CharSections::SelectionType::BASE_SKIN:
				if(section.colour_index == character.skin) {
					skin_match = true;
					break;
				}
				break;
			case dbc::CharSections::SelectionType::HAIR:
				if(section.variation_index == character.hairstyle
				   && section.colour_index == character.haircolour) {
					hair_match = true;
					break;
				}
				break;
			case dbc::CharSections::SelectionType::FACE:
				if(section.variation_index == character.face
				   && section.colour_index == character.skin) {
					face_match = true;
					break;
				}
				break;
			default: // shut the compiler up
				continue;
		}

		if(skin_match && hair_match && face_match) {
			break;
		}
	}

	// facial features (horns, markings, tusks, piercings, hair) validation
	bool facial_feature_match = false;

	for(auto& style : dbc_.character_facial_hair_styles.values()) {
		if(style.race_id == character.race && style.variation_id == character.facialhair
		   && style.sex == static_cast<dbc::CharacterFacialHairStyles::Sex>(character.gender)) {
			facial_feature_match = true;
			break;
		}
	}

	if(!facial_feature_match || !skin_match || !face_match || !hair_match) {
		LOG_WARN_FILTER(logger_, LF_NAUGHTY_USER)
			<< "Invalid visual customisation options from account " << account_id << ":"
			<< " Face ID: " << character.face
			<< " Facial feature ID: " << character.facialhair
			<< " Hair style ID: " << character.hairstyle
			<< " Hair colour ID: " << character.haircolour << LOG_ASYNC;
		return false;
	}

	return true;
}

void CharacterHandler::on_enum_complete(boost::optional<std::vector<Character>>& characters,
                                        Character& character, const ResultCB& callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!characters) {
		callback(protocol::ResultCode::CHAR_CREATE_ERROR);
		return;
	}

	if(characters->size() >= MAX_CHARACTER_SLOTS) {
		callback(protocol::ResultCode::CHAR_CREATE_SERVER_LIMIT);
		return;
	}

	// PVP faction check etc

	// everything looks good - populate the character data and create it
	const dbc::ChrRaces* race = dbc_.chr_races[character.race];
	const dbc::ChrClasses* class_ = dbc_.chr_classes[character.class_];

	auto base_info = std::find_if(dbc_.char_start_base.begin(), dbc_.char_start_base.end(), [&](auto& info) {
		return info.second.race_id == character.race && info.second.class__id == character.class_;
	});

	if(base_info == dbc_.char_start_base.end()) {
		LOG_ERROR(logger_) << "Unable to find base data for " << race->name.en_gb << " "
			<< class_->name.en_gb << LOG_ASYNC;
		callback(protocol::ResultCode::CHAR_CREATE_ERROR);
		return;
	}

	// populate zone information
	const dbc::CharStartZones* zone = base_info->second.zone;

	character.zone = zone->area_id;
	character.map = zone->area->map_id;
	character.position.x = zone->position.x;
	character.position.y = zone->position.y;
	character.position.z = zone->position.z;

	// populate items information

	// populate spells information

	// populate talents information

	const char* subzone = nullptr;

	if(zone->area->parent_area_table_id) {
		subzone = zone->area->parent_area_table->area_name.en_gb.c_str();
	}

	LOG_DEBUG(logger_) << "Creating " << race->name.en_gb << " " << class_->name.en_gb << " at "
		<< zone->area->area_name.en_gb << (subzone ? ", " : " ") << (subzone ? subzone : " ")
		<< LOG_ASYNC;

	pool_.run([=] {
		try {
			dao_.create(character);
			callback(protocol::ResultCode::CHAR_CREATE_SUCCESS);
		} catch(std::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::CHAR_CREATE_ERROR);
		}
	});
}

void CharacterHandler::name_collision_callback(const std::string& name, std::uint32_t realm_id,
                                               const ResultCB& callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=]() {
		try {
			boost::optional<Character>& res = dao_.character(name, realm_id);

			if(!res) {
				callback(protocol::ResultCode::CHAR_NAME_SUCCESS);
			} else {
				callback(protocol::ResultCode::CHAR_CREATE_NAME_IN_USE);
			}
		} catch(dal::exception& e) {
			LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
			callback(protocol::ResultCode::CHAR_CREATE_ERROR);
		}
	});
}

} // ember