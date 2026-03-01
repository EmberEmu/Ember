/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <array>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

struct ItemQuerySingleResponse final {
	std::uint32_t item;
	std::array<std::uint8_t, 482> data {
		1, 224, // size
			88, 0, // opcode (88)
			62, 28, 0, 0, // item: Item
			// Optional found
			2, 0, 0, 0, 5, 0, 0, 0, // class_and_sub_class: ItemClassAndSubClass TWO_HANDED_MACE (0x0000000500000002)
			83, 109, 105, 116, 101, 39, 115, 32, 77, 105, 103, 104, 116, 121, 32, 72, 97, 109, 109, 101, 114, 0, // name1: CString
			0, // name2: CString
			0, // name3: CString
			0, // name4: CString
			154, 76, 0, 0, // display_id: u32
			3, 0, 0, 0, // quality: ItemQuality RARE (3)
			0, 0, 0, 0, // flags: ItemFlag  NONE (0)
			155, 60, 0, 0, // buy_price: Gold
			31, 12, 0, 0, // sell_price: Gold
			17, 0, 0, 0, // inventory_type: InventoryType TWO_HANDED_WEAPON (17)
			223, 5, 0, 0, // allowed_class: AllowedClass  WARRIOR| PALADIN| HUNTER| ROGUE| PRIEST| SHAMAN| MAGE| WARLOCK| DRUID (1503)
			255, 1, 0, 0, // allowed_race: AllowedRace  HUMAN| ORC| DWARF| NIGHT_ELF| UNDEAD| TAUREN| GNOME| TROLL| GOBLIN (511)
			23, 0, 0, 0, // item_level: Level32
			18, 0, 0, 0, // required_level: Level32
			0, 0, 0, 0, // required_skill: Skill NONE (0)
			0, 0, 0, 0, // required_skill_rank: u32
			0, 0, 0, 0, // required_spell: Spell
			0, 0, 0, 0, // required_honor_rank: u32
			0, 0, 0, 0, // required_city_rank: u32
			0, 0, 0, 0, // required_faction: Faction NONE (0)
			0, 0, 0, 0, // required_faction_rank: u32
			0, 0, 0, 0, // max_count: u32
			1, 0, 0, 0, // stackable: u32
			0, 0, 0, 0, // container_slots: u32
			0, 0, 0, 0, // [0].ItemStat.stat_type: ItemStatType MANA (0)
			0, 0, 0, 0, // [0].ItemStat.value: i32
			1, 0, 0, 0, // [1].ItemStat.stat_type: ItemStatType HEALTH (1)
			0, 0, 0, 0, // [1].ItemStat.value: i32
			4, 0, 0, 0, // [2].ItemStat.stat_type: ItemStatType STRENGTH (4)
			11, 0, 0, 0, // [2].ItemStat.value: i32
			3, 0, 0, 0, // [3].ItemStat.stat_type: ItemStatType AGILITY (3)
			4, 0, 0, 0, // [3].ItemStat.value: i32
			7, 0, 0, 0, // [4].ItemStat.stat_type: ItemStatType STAMINA (7)
			0, 0, 0, 0, // [4].ItemStat.value: i32
			5, 0, 0, 0, // [5].ItemStat.stat_type: ItemStatType INTELLECT (5)
			0, 0, 0, 0, // [5].ItemStat.value: i32
			6, 0, 0, 0, // [6].ItemStat.stat_type: ItemStatType SPIRIT (6)
			0, 0, 0, 0, // [6].ItemStat.value: i32
			0, 0, 0, 0, // [7].ItemStat.stat_type: ItemStatType MANA (0)
			0, 0, 0, 0, // [7].ItemStat.value: i32
			0, 0, 0, 0, // [8].ItemStat.stat_type: ItemStatType MANA (0)
			0, 0, 0, 0, // [8].ItemStat.value: i32
			0, 0, 0, 0, // [9].ItemStat.stat_type: ItemStatType MANA (0)
			0, 0, 0, 0, // [9].ItemStat.value: i32
			// stats: ItemStat[10]
			0, 0, 92, 66, // [0].ItemDamageType.damage_minimum: f32
			0, 0, 166, 66, // [0].ItemDamageType.damage_maximum: f32
			0, 0, 0, 0, // [0].ItemDamageType.school: SpellSchool NORMAL (0)
			0, 0, 0, 0, // [1].ItemDamageType.damage_minimum: f32
			0, 0, 0, 0, // [1].ItemDamageType.damage_maximum: f32
			0, 0, 0, 0, // [1].ItemDamageType.school: SpellSchool NORMAL (0)
			0, 0, 0, 0, // [2].ItemDamageType.damage_minimum: f32
			0, 0, 0, 0, // [2].ItemDamageType.damage_maximum: f32
			0, 0, 0, 0, // [2].ItemDamageType.school: SpellSchool NORMAL (0)
			0, 0, 0, 0, // [3].ItemDamageType.damage_minimum: f32
			0, 0, 0, 0, // [3].ItemDamageType.damage_maximum: f32
			0, 0, 0, 0, // [3].ItemDamageType.school: SpellSchool NORMAL (0)
			0, 0, 0, 0, // [4].ItemDamageType.damage_minimum: f32
			0, 0, 0, 0, // [4].ItemDamageType.damage_maximum: f32
			0, 0, 0, 0, // [4].ItemDamageType.school: SpellSchool NORMAL (0)
			// damages: ItemDamageType[5]
			0, 0, 0, 0, // armor: i32
			0, 0, 0, 0, // holy_resistance: i32
			0, 0, 0, 0, // fire_resistance: i32
			0, 0, 0, 0, // nature_resistance: i32
			0, 0, 0, 0, // frost_resistance: i32
			0, 0, 0, 0, // shadow_resistance: i32
			0, 0, 0, 0, // arcane_resistance: i32
			172, 13, 0, 0, // delay: u32
			0, 0, 0, 0, // ammo_type: u32
			0, 0, 0, 0, // ranged_range_modification: f32
			0, 0, 0, 0, // [0].ItemSpells.spell: Spell
			0, 0, 0, 0, // [0].ItemSpells.spell_trigger: SpellTriggerType ON_USE (0)
			0, 0, 0, 0, // [0].ItemSpells.spell_charges: i32
			0, 0, 0, 0, // [0].ItemSpells.spell_cooldown: i32
			0, 0, 0, 0, // [0].ItemSpells.spell_category: u32
			0, 0, 0, 0, // [0].ItemSpells.spell_category_cooldown: i32
			0, 0, 0, 0, // [1].ItemSpells.spell: Spell
			0, 0, 0, 0, // [1].ItemSpells.spell_trigger: SpellTriggerType ON_USE (0)
			0, 0, 0, 0, // [1].ItemSpells.spell_charges: i32
			0, 0, 0, 0, // [1].ItemSpells.spell_cooldown: i32
			0, 0, 0, 0, // [1].ItemSpells.spell_category: u32
			0, 0, 0, 0, // [1].ItemSpells.spell_category_cooldown: i32
			0, 0, 0, 0, // [2].ItemSpells.spell: Spell
			0, 0, 0, 0, // [2].ItemSpells.spell_trigger: SpellTriggerType ON_USE (0)
			0, 0, 0, 0, // [2].ItemSpells.spell_charges: i32
			0, 0, 0, 0, // [2].ItemSpells.spell_cooldown: i32
			0, 0, 0, 0, // [2].ItemSpells.spell_category: u32
			0, 0, 0, 0, // [2].ItemSpells.spell_category_cooldown: i32
			0, 0, 0, 0, // [3].ItemSpells.spell: Spell
			0, 0, 0, 0, // [3].ItemSpells.spell_trigger: SpellTriggerType ON_USE (0)
			0, 0, 0, 0, // [3].ItemSpells.spell_charges: i32
			0, 0, 0, 0, // [3].ItemSpells.spell_cooldown: i32
			0, 0, 0, 0, // [3].ItemSpells.spell_category: u32
			0, 0, 0, 0, // [3].ItemSpells.spell_category_cooldown: i32
			0, 0, 0, 0, // [4].ItemSpells.spell: Spell
			0, 0, 0, 0, // [4].ItemSpells.spell_trigger: SpellTriggerType ON_USE (0)
			0, 0, 0, 0, // [4].ItemSpells.spell_charges: i32
			0, 0, 0, 0, // [4].ItemSpells.spell_cooldown: i32
			0, 0, 0, 0, // [4].ItemSpells.spell_category: u32
			0, 0, 0, 0, // [4].ItemSpells.spell_category_cooldown: i32
			// spells: ItemSpells[5]
			1, 0, 0, 0, // bonding: Bonding PICK_UP (1)
			0, // description: CString
			0, 0, 0, 0, // page_text: u32
			0, 0, 0, 0, // language: Language UNIVERSAL (0)
			0, 0, 0, 0, // page_text_material: PageTextMaterial NONE (0)
			0, 0, 0, 0, // start_quest: u32
			0, 0, 0, 0, // lock_id: u32
			2, 0, 0, 0, // material: u32
			1, 0, 0, 0, // sheathe_type: SheatheType MAIN_HAND (1)
			0, 0, 0, 0, // random_property: u32
			0, 0, 0, 0, // block: u32
			0, 0, 0, 0, // item_set: ItemSet NONE (0)
			80, 0, 0, 0, // max_durability: u32
			0, 0, 0, 0, // area: Area NONE (0)
			0, 0, 0, 0, // map: Map EASTERN_KINGDOMS (0)
			0, 0, 0, 0, // bag_family: BagFamily NONE (0)
	};

	StreamResult read_from_stream(auto& stream) try {
		stream >> item;
		stream >> data;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(auto& stream) const try {
		stream << item;
		stream << data;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // server, protocol, ember
