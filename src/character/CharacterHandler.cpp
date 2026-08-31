/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterHandler.h"
#include "FilterTypes.h"
#include <logger/Logger.h>
#include <shared/utility/Utility.h>
#include <thread/ThreadPool.h>
#include <boost/assert.hpp>

namespace ember {

void CharacterHandler::create(std::uint32_t account_id, std::uint32_t realm_id,
                              const rpc::Character::CharacterTemplate& options,
                              ResultCB callback) const {
	LOG_TRACE(logger_, log_func);

	Character character{};
	character.race = options.race();
	const auto char_name = options.name()->c_str();
	character.name = char_name;
	character.internal_name = char_name;
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
	character.flags = Character::Flags::none;
	character.first_login = true;

	pool_.run([=, this] {
		do_create(account_id, realm_id, character, callback);
	});
}

void CharacterHandler::restore(std::uint64_t id, ResultCB callback) const {
	LOG_TRACE(logger_, log_func);

	pool_.run([=, this] {
		do_restore(id, callback);
	});
}

void CharacterHandler::erase(std::uint32_t account_id, std::uint32_t realm_id,
                             std::uint64_t character_id, ResultCB callback) const {
	LOG_TRACE(logger_, log_func);

	pool_.run([=, this] {
		do_erase(account_id, realm_id, character_id, callback);
	});
}

void CharacterHandler::enumerate(std::uint32_t account_id, std::uint32_t realm_id,
                                 EnumResultCB callback) const {
	LOG_TRACE(logger_, log_func);

	pool_.run([=, this] {
		do_enumerate(account_id, realm_id, callback);
	});
}

void CharacterHandler::rename(std::uint32_t account_id, std::uint64_t character_id,
                              const utf8_string& name, RenameCB callback) const {
	LOG_TRACE(logger_, log_func);

	pool_.run([=, this] {
		do_rename(account_id, character_id, name, callback);
	});
}

void CharacterHandler::do_create(std::uint32_t account_id, std::uint32_t realm_id,
                                 Character character, const ResultCB& callback) const try {
	LOG_TRACE(logger_, log_func);

	// class, race and visual customisation validation
	const bool success = validate_options(character, account_id);

	if(!success) {
		callback(protocol::Result::char_create_error);
		return;
	}

	// name validation
	auto result = validate_name(character.name);

	if(result != protocol::Result::char_name_success) {
		callback(result);
		return;
	}

	character.name = utility::utf8::name_format(character.name, std::locale());

	const auto res = dao_.character(character.name, realm_id);

	if(res) {
		callback(protocol::Result::char_create_name_in_use);
		return;
	}

	// query database for further validation steps
	const auto total_chars = dao_.count(account_id);

	if(total_chars >= config_.max_chars_slots_account) {
		callback(protocol::Result::char_create_account_limit);
		return;
	}

	// avoid an additional query if there's no need for it
	if(total_chars >= config_.max_chars_slots_server) {
		const auto count = dao_.count(account_id, realm_id);

		if(count >= config_.max_chars_slots_server) {
			callback(protocol::Result::char_create_server_limit);
			return;
		}
	}

	const auto& characters = dao_.characters(account_id, realm_id);

	// PvP faction check
	auto faction_group = dbc_.chr_races[character.race]->faction->faction_group_id;

	auto it = std::ranges::find_if_not(characters, [&](const auto& c) {
		return faction_group == dbc_.chr_races[c.race]->faction->faction_group_id;
	});

	if(it != characters.end() /* && pvp_server */) { // todo, add check to make sure it's a PvP server
		auto current = pvp_faction(*dbc_.chr_races[characters.front().race]->faction);
		auto opposing = pvp_faction(*dbc_.chr_races[character.race]->faction);

		LOG_DEBUG(logger_, "Cannot create {} characters with existing {} characters on a PvP realm",
		                opposing->internal_name, current->internal_name);

		callback(protocol::Result::char_create_pvp_teams_violation);
		return;
	}

	// everything looks good - populate the character data and create it
	const dbc::ChrRaces* race = dbc_.chr_races[character.race];
	const dbc::ChrClasses* class_ = dbc_.chr_classes[character.class_];

	auto base_info = std::ranges::find_if(dbc_.char_start_base, [&](const auto& record) {
		return record.second.race_id == character.race
			&& record.second.class__id == character.class_;
	});

	if(base_info == dbc_.char_start_base.end()) {
		LOG_ERROR(logger_, "Unable to find base data for {} {}",
		                race->name.en_gb, class_->name.en_gb);
		callback(protocol::Result::char_create_error);
		return;
	}

	// defer zone placement until first world entry (original retail behaviour)
	const auto zone = base_info->second.zone;

	if(!zone) {
		LOG_ERROR(logger_, "Unable to find zone data for {} {}",
		                race->name.en_gb, class_->name.en_gb);
		callback(protocol::Result::char_create_error);
		return;
	}
	
	if(!config_.defer_zone_placement) {
		populate_zone(character, *zone);
	}
	
	// populate starting equipment
	const auto& items = std::ranges::find_if(dbc_.char_start_outfit, [&](const auto& record) {
		return record.second.race_id == character.race
			&& record.second.class__id == character.class_;
	});

	if(items != dbc_.char_start_outfit.end()) {
		populate_items(character, items->second);
	} else { // could be intentional, so we'll keep going
		LOG_DEBUG(logger_, "No starting item data found for {}, {}",
		                race->name.en_gb, class_->name.en_gb);
	}

	// populate starting spells
	const auto& spells = std::ranges::find_if(dbc_.char_start_spells, [&](const auto& record) {
		return record.second.race_id == character.race && record.second.class__id == character.class_;
	});

	if(spells != dbc_.char_start_spells.end()) {
		populate_spells(character, spells->second);
	} else { // could be intentional, so we'll keep going
		LOG_DEBUG(logger_, "No starting spell data found for {} {}",
		                race->name.en_gb, class_->name.en_gb);
	}

	// populate starting skills
	const auto& skills = std::ranges::find_if(dbc_.char_start_skills, [&](const auto& record) {
		return record.second.race_id == character.race
			&& record.second.class__id == character.class_;
	});

	if(skills != dbc_.char_start_skills.end()) {
		populate_skills(character, skills->second);
	} else { // could be intentional, so we'll keep going
		LOG_DEBUG(logger_, "No starting skill data found for {} {}",
		                race->name.en_gb, class_->name.en_gb);
	}

	const char* subzone = nullptr;

	if(zone->area->parent_area_table_id) {
		subzone = zone->area->parent_area_table->area_name.en_gb.c_str();
	}

	LOG_DEBUG(logger_, "Creating {} {} at {}{} {}", 
	                race->name.en_gb,
	                class_->name.en_gb,
	                zone->area->area_name.en_gb,
	                subzone? "," : " ",
	                subzone? subzone : " ");

	dao_.create(character);
	callback(protocol::Result::char_create_success);
} catch(const dal::exception& e) {
	LOG_ERROR(logger_, e.what());
	callback(protocol::Result::char_create_error);
}

void CharacterHandler::do_erase(std::uint32_t account_id, std::uint32_t realm_id,
                                std::uint64_t character_id, const ResultCB& callback) const try {
	LOG_TRACE(logger_, log_func);

	auto character = dao_.character(character_id);
	
	// character must exist, belong to the same account and be on the same realm
	if(!character || character->account_id != account_id || character->realm_id != realm_id) {
		LOG_DEBUG(logger_, "Account {} attempted an invalid delete on character {}", account_id, character_id);
		callback(protocol::Result::char_delete_failed);
		return;
	}

	if((character->flags & Character::Flags::locked_for_transfer) == Character::Flags::locked_for_transfer) {
		callback(protocol::Result::char_delete_failed_locked_for_transfer);
		return;
	}

	// character cannot be a guild leader (no specific guild leader deletion message until TBC)
	if(character->guild_rank == 1) { // todo, ranks need defined properly
		callback(protocol::Result::char_delete_failed);
		return;
	}

	LOG_DEBUG(logger_, "Deleting {}, #{}", character->name, character->id);

	dao_.delete_character(character_id, true);
	callback(protocol::Result::char_delete_success);
} catch(const dal::exception& e) {
	LOG_ERROR(logger_, e.what());
	callback(protocol::Result::char_delete_failed);
}

void CharacterHandler::do_enumerate(std::uint32_t account_id, std::uint32_t realm_id,
                                    const EnumResultCB& callback) const try {
	LOG_TRACE(logger_, log_func);

	auto characters = dao_.characters(account_id, realm_id);
	callback(true, std::move(characters));
} catch(const dal::exception& e) {
	LOG_ERROR(logger_, e.what());
	callback(false, {});
}

void CharacterHandler::do_rename(std::uint32_t account_id, std::uint64_t character_id,
                                 const utf8_string& name, const RenameCB& callback) const try {
	LOG_TRACE(logger_, log_func);

	auto character = dao_.character(character_id);
	
	if(!character) {
		callback(protocol::Result::char_name_failure, std::nullopt);
		return;
	}

	if(character->account_id != account_id) {
		callback(protocol::Result::char_name_failure, std::nullopt);
		return;
	}

	if((character->flags & Character::Flags::rename) != Character::Flags::rename) {
		callback(protocol::Result::char_name_failure, std::nullopt);
		return;
	}

	auto result = validate_name(name);

	if(result != protocol::Result::char_name_success) {
		callback(result, std::nullopt);
		return;
	}

	character->name = utility::utf8::name_format(name, std::locale());

	const std::optional<Character>& match = dao_.character(character->name, character->realm_id);

	if(match) {
		callback(protocol::Result::char_create_name_in_use, std::nullopt);
		return;
	}
	
	LOG_DEBUG(logger_, "Renaming {} => {}, #{}", character->name, name, character->id);

	character->internal_name = character->name;
	character->flags ^= Character::Flags::rename;

	dao_.update(*character);
	callback(protocol::Result::response_success, *character);
} catch(const dal::exception& e) {
	LOG_ERROR(logger_, e.what());
	callback(protocol::Result::char_name_failure, std::nullopt);
}

void CharacterHandler::do_restore(std::uint64_t id, const ResultCB& callback) const try {
	LOG_TRACE(logger_, log_func);

	auto character = dao_.character(id);

	if(!character) {
		LOG_WARN(logger_, "Cannot restore character - character not found");
		callback(protocol::Result::response_failure);
		return;
	}

	auto characters = dao_.characters(character->account_id);

	if(characters.size() >= config_.max_chars_slots_account) {
		LOG_WARN(logger_, "Cannot restore character - would exceed max account slots");
		callback(protocol::Result::char_create_account_limit);
		return;
	}

	auto realm_chars = std::ranges::count_if(characters, [&](const auto& c) {
		return c.realm_id == character->realm_id;
	});

	if(realm_chars >= config_.max_chars_slots_server) {
		LOG_WARN(logger_, "Cannot restore character - would exceed max server slots");
		callback(protocol::Result::char_create_server_limit);
		return;
	}

	// ensure their name hasn't been taken - if so, force a rename
	const auto& name_taken = dao_.character(character->name, character->realm_id);

	if(name_taken) {
		character->flags |= Character::Flags::rename;
	} else {
		character->internal_name = character->name;
	}

	LOG_DEBUG(logger_, "Restoring {}, #{}", character->name, character->id);

	dao_.update(*character);
	dao_.restore(id);
	callback(protocol::Result::response_success);
} catch(const dal::exception& e) {
	LOG_ERROR(logger_, e.what());
	callback(protocol::Result::response_failure);
}

bool CharacterHandler::validate_options(const Character& character, std::uint32_t account_id) const {
	LOG_TRACE(logger_, log_func);

	// validate the race/class combination
	auto found = std::ranges::find_if(dbc_.char_base_info, [&](auto val) {
		return (character.class_ == val.second.class__id && character.race == val.second.race_id);
	});

	if(found == dbc_.char_base_info.end()) {
		LOG_DEBUG(logger_, "Invalid race/class combination of {} {} from account ID {}",
		                character.race, character.class_, account_id);
		return false;
	}

	bool skin_match = false;
	bool hair_match = false;
	bool face_match = false;

	// validate visual customisation options
	for(auto&& [k, section] : dbc_.char_sections) {
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

	for(auto&& [k, style] : dbc_.character_facial_hair_styles) {
		if(style.race_id == character.race && style.variation_id == character.facialhair
		   && style.sex == static_cast<dbc::CharacterFacialHairStyles::Sex>(character.gender)) {
			facial_feature_match = true;
			break;
		}
	}

	if(!facial_feature_match || !skin_match || !face_match || !hair_match) {
		LOG_DEBUG(logger_, "Invalid visual customisation options, account {} - "
		               "Face ID: {}, facial feature ID: {}, hair style ID: {}, hair colour ID: {}",
			            account_id, character.face, character.facialhair, character.hairstyle, character.haircolour);
		return false;
	}

	return true;
}

protocol::Result CharacterHandler::validate_name(const utf8_string& name) const {
	LOG_TRACE(logger_, log_func);

	if(name.empty()) {
		return protocol::Result::char_name_no_name;
	}

	if(!utility::utf8::is_valid(name)) {
		return protocol::Result::char_name_failure;
	}
	
	const std::size_t length = utility::utf8::length(name);

	if(length > MAX_NAME_LENGTH) {
		return protocol::Result::char_name_too_long;
	}

	if(length < MIN_NAME_LENGTH) {
		return protocol::Result::char_name_too_short;
	}

	// todo, add a config option to restrict names to ASCII

	if(utility::utf8::max_consecutive(name, true) > MAX_CONSECUTIVE_LETTERS) {
		return protocol::Result::char_name_three_consecutive;
	}

	if(!utility::utf8::is_alpha(name, std::locale())) {
		return protocol::Result::char_name_only_letters;
	}

	const auto& formatted_name = utility::utf8::name_format(name, std::locale());

	for(auto& regex : reserved_names_) {
		int ret = utility::pcre::match(formatted_name, regex);

		if(ret >= 0) {
			return protocol::Result::char_name_reserved;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_, "PCRE error encountered: {}", ret);
			return protocol::Result::char_name_failure;
		}
	}

	for(auto& regex : profane_names_) {
		int ret = utility::pcre::match(formatted_name, regex);

		if(ret >= 0) {
			return protocol::Result::char_name_profane;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_, "PCRE error encountered: {}", ret);
			return protocol::Result::char_name_failure;
		}
	}

	for(auto& regex : spam_names_) {
		int ret = utility::pcre::match(formatted_name, regex);

		if(ret >= 0) {
			return protocol::Result::char_name_reserved;
		} else if(ret != PCRE_ERROR_NOMATCH) {
			LOG_ERROR(logger_, "PCRE error encountered: {}", ret);
			return protocol::Result::char_name_failure;
		}
	}

	return protocol::Result::char_name_success;
}

// This function should be moved when there's a more suitable home for it
const dbc::FactionGroup* CharacterHandler::pvp_faction(const dbc::FactionTemplate& fac_template) const {
	for(auto&& [k, group] : dbc_.faction_group) {
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

void CharacterHandler::populate_zone(Character& character, const dbc::CharStartZones& zone) const {
	character.zone = zone.area_id;
	character.map = zone.area->map_id;
	character.position.x = zone.position.x;
	character.position.y = zone.position.y;
	character.position.z = zone.position.z;
	character.orientation = zone.orientation;
}

void CharacterHandler::populate_items(Character& character, const dbc::CharStartOutfit& outfit) const {
}

void CharacterHandler::populate_spells(Character& character, const dbc::CharStartSpells& spells) const {
}

void CharacterHandler::populate_skills(Character& character, const dbc::CharStartSkills& skills) const {
}

} // ember