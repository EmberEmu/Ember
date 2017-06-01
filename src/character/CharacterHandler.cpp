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
#include <boost/assert.hpp>

namespace ember {

void CharacterHandler::create(std::uint32_t account_id, std::uint32_t realm_id,
                              const messaging::character::CharacterTemplate& options,
                              ResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	Character character{};
	character.race = options.race();
	character.name = options.name()->c_str();
	character.internal_name = options.name()->c_str();
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
	character.flags = Character::Flags::NONE;
	character.first_login = true;

	pool_.run([=] {
		do_create(account_id, realm_id, character, callback);
	});
}

void CharacterHandler::restore(std::uint64_t id, ResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		do_restore(id, callback);
	});
}

void CharacterHandler::erase(std::uint32_t account_id, std::uint32_t realm_id,
                             std::uint64_t character_id, ResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		do_erase(account_id, realm_id, character_id, callback);
	});
}

void CharacterHandler::enumerate(std::uint32_t account_id, std::uint32_t realm_id,
                                 EnumResultCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		do_enumerate(account_id, realm_id, callback);
	});
}

void CharacterHandler::rename(std::uint32_t account_id, std::uint64_t character_id,
                              const std::string& name, RenameCB callback) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	pool_.run([=] {
		do_rename(account_id, character_id, name, callback);
	});
}

void CharacterHandler::do_create(std::uint32_t account_id, std::uint32_t realm_id,
                                 Character character, const ResultCB& callback) const try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	// class, race and visual customisation validation
	bool success = validate_options(character, account_id);

	if(!success) {
		callback(protocol::Result::CHAR_CREATE_ERROR);
		return;
	}

	// name validation
	auto result = validate_name(character.name);

	if(result != protocol::Result::CHAR_NAME_SUCCESS) {
		callback(result);
		return;
	}

	const std::optional<Character>& res = dao_.character(character.name, realm_id);

	if(res) {
		callback(protocol::Result::CHAR_CREATE_NAME_IN_USE);
		return;
	}

	// query database for further validation steps
	const auto& characters = dao_.characters(account_id);

	if(characters.size() >= MAX_CHARACTER_SLOTS_ACCOUNT) {
		callback(protocol::Result::CHAR_CREATE_ACCOUNT_LIMIT);
		return;
	}

	auto realm_chars = std::count_if(characters.begin(), characters.end(), [&](auto& c) {
		return c.realm_id == realm_id;
	});

	if(static_cast<std::size_t>(realm_chars) >= MAX_CHARACTER_SLOTS_SERVER) {
		callback(protocol::Result::CHAR_CREATE_SERVER_LIMIT);
		return;
	}

	// PvP faction check
	auto faction_group = dbc_.chr_races[character.race]->faction->faction_group_id;

	auto it = std::find_if_not(characters.begin(), characters.end(), [&](auto& c) {
		return faction_group == dbc_.chr_races[c.race]->faction->faction_group_id;
	});

	if(it != characters.end() /* && pvp_server */) { // todo, add check to make sure it's a PvP server
		auto current = pvp_faction(*dbc_.chr_races[characters.front().race]->faction);
		auto opposing = pvp_faction(*dbc_.chr_races[character.race]->faction);

		LOG_DEBUG(logger_) << "Cannot create " << opposing->internal_name
			<< " characters with existing " << current->internal_name
			<< " characters on a PvP realm" << LOG_ASYNC;

		callback(protocol::Result::CHAR_CREATE_PVP_TEAMS_VIOLATION);
		return;
	}

	// everything looks good - populate the character data and create it
	const dbc::ChrRaces* race = dbc_.chr_races[character.race];
	const dbc::ChrClasses* class_ = dbc_.chr_classes[character.class_];

	auto base_info = std::find_if(dbc_.char_start_base.begin(), dbc_.char_start_base.end(), [&](auto& record) {
		return record.second.race_id == character.race && record.second.class__id == character.class_;
	});

	if(base_info == dbc_.char_start_base.end()) {
		LOG_ERROR(logger_) << "Unable to find base data for " << race->name.en_gb << " "
			<< class_->name.en_gb << LOG_ASYNC;
		callback(protocol::Result::CHAR_CREATE_ERROR);
		return;
	}

	// populate zone information
	const dbc::CharStartZones* zone = base_info->second.zone;

	character.zone = zone->area_id;
	character.map = zone->area->map_id;
	character.position.x = zone->position.x;
	character.position.y = zone->position.y;
	character.position.z = zone->position.z;

	// populate starting equipment
	auto items = std::find_if(dbc_.char_start_outfit.begin(), dbc_.char_start_outfit.end(), [&](auto& record) {
		return record.second.race_id == character.race && record.second.class__id == character.class_;
	});

	if(items != dbc_.char_start_outfit.end()) {
		populate_items(character, items->second);
	} else { // could be intentional, so we'll keep going
		LOG_DEBUG(logger_) << "No starting item data found for race" <<
			character.race << ", class " << character.class_ << LOG_ASYNC;
	}

	// populate spells information

	// populate talents information

	const char* subzone = nullptr;

	if(zone->area->parent_area_table_id) {
		subzone = zone->area->parent_area_table->area_name.en_gb.c_str();
	}

	LOG_DEBUG(logger_) << "Creating " << race->name.en_gb << " " << class_->name.en_gb << " at "
		<< zone->area->area_name.en_gb << (subzone ? ", " : " ") << (subzone ? subzone : " ")
		<< LOG_ASYNC;

	dao_.create(character);
	callback(protocol::Result::CHAR_CREATE_SUCCESS);
} catch(dal::exception& e) {
	LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
	callback(protocol::Result::CHAR_CREATE_ERROR);
}

void CharacterHandler::do_erase(std::uint32_t account_id, std::uint32_t realm_id,
                                std::uint64_t character_id, const ResultCB& callback) const try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto character = dao_.character(character_id);
	
	// character must exist, belong to the same account and be on the same realm
	if(!character || character->account_id != account_id || character->realm_id != realm_id) {
		LOG_WARN_FILTER(logger_, LF_NAUGHTY_USER)
			<< "Account " << account_id << " attempted an invalid delete on character "
			<< character_id << LOG_ASYNC;
		callback(protocol::Result::CHAR_DELETE_FAILED);
		return;
	}

	if((character->flags & Character::Flags::LOCKED_FOR_TRANSFER) == Character::Flags::LOCKED_FOR_TRANSFER) {
		callback(protocol::Result::CHAR_DELETE_FAILED_LOCKED_FOR_TRANSFER);
		return;
	}

	// character cannot be a guild leader (no specific guild leader deletion message until TBC)
	if(character->guild_rank == 1) { // todo, ranks need defined properly
		callback(protocol::Result::CHAR_DELETE_FAILED);
		return;
	}

	LOG_DEBUG(logger_) << "Deleting " << character->name << ", #" << character->id << LOG_ASYNC;

	dao_.delete_character(character_id, true);
	callback(protocol::Result::CHAR_DELETE_SUCCESS);
} catch(dal::exception& e) {
	LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
	callback(protocol::Result::CHAR_DELETE_FAILED);
}

void CharacterHandler::do_enumerate(std::uint32_t account_id, std::uint32_t realm_id,
                                    const EnumResultCB& callback) const try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto characters = dao_.characters(account_id, realm_id);
	callback(std::move(characters));
} catch(dal::exception& e) {
	LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
	callback(std::optional<std::vector<Character>>());
}

void CharacterHandler::do_rename(std::uint32_t account_id, std::uint64_t character_id,
                                 const std::string& name, const RenameCB& callback) const try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto character = dao_.character(character_id);
	
	if(!character) {
		callback(protocol::Result::CHAR_NAME_FAILURE, std::nullopt);
		return;
	}

	if(character->account_id != account_id) {
		callback(protocol::Result::CHAR_NAME_FAILURE, std::nullopt);
		return;
	}

	if((character->flags & Character::Flags::RENAME) != Character::Flags::RENAME) {
		callback(protocol::Result::CHAR_NAME_FAILURE, std::nullopt);
		return;
	}

	auto result = validate_name(name);

	if(result != protocol::Result::CHAR_NAME_SUCCESS) {
		callback(result, std::nullopt);
		return;
	}

	const std::optional<Character>& match = dao_.character(name, character->realm_id);

	if(match) {
		callback(protocol::Result::CHAR_CREATE_NAME_IN_USE, std::nullopt);
		return;
	}

	LOG_DEBUG(logger_) << "Renaming " << character->name << " => " << name << ", #"
		<< character->id << LOG_ASYNC;

	character->name = name;
	character->internal_name = name;
	character->flags ^= Character::Flags::RENAME;

	dao_.update(*character);
	callback(protocol::Result::RESPONSE_SUCCESS, *character);
} catch(dal::exception& e) {
	LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
	callback(protocol::Result::CHAR_NAME_FAILURE, std::nullopt);
}

void CharacterHandler::do_restore(std::uint64_t id, const ResultCB& callback) const try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto character = dao_.character(id);
	auto characters = dao_.characters(character->account_id);

	if(characters.size() >= MAX_CHARACTER_SLOTS_ACCOUNT) {
		LOG_WARN(logger_) << "Cannot restore character - would exceed max account slots" << LOG_ASYNC;
		callback(protocol::Result::CHAR_CREATE_ACCOUNT_LIMIT);
		return;
	}

	auto realm_chars = std::count_if(characters.begin(), characters.end(), [&](auto& c) {
		return c.realm_id == character->realm_id;
	});

	if(static_cast<std::size_t>(realm_chars) >= MAX_CHARACTER_SLOTS_SERVER) {
		LOG_WARN(logger_) << "Cannot restore character - would exceed max server slots" << LOG_ASYNC;
		callback(protocol::Result::CHAR_CREATE_SERVER_LIMIT);
		return;
	}

	// ensure their name hasn't been taken - if so, force a rename
	const auto& name_taken = dao_.character(character->name, character->realm_id);

	if(name_taken) {
		character->flags |= Character::Flags::RENAME;
	} else {
		character->internal_name = character->name;
	}

	LOG_DEBUG(logger_) << "Restoring " << character->name << ", #" << character->id << LOG_ASYNC;

	dao_.update(*character);
	dao_.restore(id);
	callback(protocol::Result::RESPONSE_SUCCESS);
} catch(dal::exception& e) {
	LOG_ERROR(logger_) << e.what() << LOG_ASYNC;
	callback(protocol::Result::RESPONSE_FAILURE);
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

protocol::Result CharacterHandler::validate_name(const std::string& name) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(name.empty()) {
		return protocol::Result::CHAR_NAME_NO_NAME;
	}

	bool valid = false;
	std::size_t name_length = util::utf8::length(name, valid);

	if(!valid) { // wasn't a valid UTF-8 encoded string
		return protocol::Result::CHAR_NAME_FAILURE;
	}

	if(name_length > MAX_NAME_LENGTH) {
		return protocol::Result::CHAR_NAME_TOO_LONG;
	}

	if(name_length < MIN_NAME_LENGTH) {
		return protocol::Result::CHAR_NAME_TOO_SHORT;
	}

	if(util::max_consecutive_check(name) > MAX_CONSECUTIVE_LETTERS) { // todo, not unicode aware
		return protocol::Result::CHAR_NAME_THREE_CONSECUTIVE;
	}

	// this is probably all horribly wrong
	//std::vector<wchar_t> utf16_name;
	//utf8::unchecked::utf8to16(name.begin(), name.end(), std::back_inserter(utf16_name));

	//bool alpha_only = std::find_if(utf16_name.begin(), utf16_name.end(), [&](wchar_t c) {
	//	return !std::iswalpha(c);
	//}) != utf16_name.end();

	//if(!alpha_only) {
	//	return protocol::Result::CHAR_NAME_ONLY_LETTERS;
	//}

	for(auto& regex : reserved_names_) {
		int ret = util::pcre::match(name, regex);

		if(ret >= 0) {
			return protocol::Result::CHAR_NAME_RESERVED;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_) << "PCRE error encountered: " + std::to_string(ret) << LOG_ASYNC;
			return protocol::Result::CHAR_NAME_FAILURE;
		}
	}

	for(auto& regex : profane_names_) {
		int ret = util::pcre::match(name, regex);

		if(ret >= 0) {
			return protocol::Result::CHAR_NAME_PROFANE;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_) << "PCRE error encountered: " + std::to_string(ret) << LOG_ASYNC;
			return protocol::Result::CHAR_NAME_FAILURE;
		}
	}

	for(auto& regex : spam_names_) {
		int ret = util::pcre::match(name, regex);

		if(ret >= 0) {
			return protocol::Result::CHAR_NAME_RESERVED;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_) << "PCRE error encountered: " + std::to_string(ret) << LOG_ASYNC;
			return protocol::Result::CHAR_NAME_FAILURE;
		}
	}

	return protocol::Result::CHAR_NAME_SUCCESS;
}

// This function should be moved when there's a more suitable home for it
const dbc::FactionGroup* CharacterHandler::pvp_faction(const dbc::FactionTemplate& fac_template) const {
	for(auto& group : dbc_.faction_group.values()) {
		if(group.internal_name == "Player") {
			if(fac_template.faction_group_id == (1 << group.mask_id)) {
				return &group;
			}
		}

		if(group.mask_id) {
			if(fac_template.faction_group_id & (1 << group.mask_id)) {
				return &group;
			}
		}
	}

	return nullptr;
}

void CharacterHandler::populate_items(Character& character, const dbc::CharStartOutfit& outfit) const {

}

} // ember