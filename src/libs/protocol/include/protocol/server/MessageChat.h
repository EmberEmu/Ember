/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <protocol/ResultCodes.h>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

enum ChatType : std::uint8_t {
	SAY = 0x00,
	PARTY = 0x01,
	RAID = 0x02,
	GUILD = 0x03,
	OFFICER = 0x04,
	YELL = 0x05,
	WHISPER = 0x06,
	WHISPER_INFORM = 0x07,
	EMOTE = 0x08,
	TEXT_EMOTE = 0x09,
	SYSTEM = 0x0A,
	MONSTER_SAY = 0x0B,
	MONSTER_YELL = 0x0C,
	MONSTER_EMOTE = 0x0D,
	CHANNEL = 0x0E,
	CHANNEL_JOIN = 0x0F,
	CHANNEL_LEAVE = 0x10,
	CHANNEL_LIST = 0x11,
	CHANNEL_NOTICE = 0x12,
	CHANNEL_NOTICE_USER = 0x13,
	AFK = 0x14,
	DND = 0x15,
	IGNORED = 0x16,
	SKILL = 0x17,
	LOOT = 0x18,
	MONSTER_WHISPER = 0x1A,
	BG_SYSTEM_NEUTRAL = 0x52,
	BG_SYSTEM_ALLIANCE = 0x53,
	BG_SYSTEM_HORDE = 0x54,
	RAID_LEADER = 0x57,
	RAID_WARNING = 0x58,
	RAID_BOSS_WHISPER = 0x59,
	RAID_BOSS_EMOTE = 0x5A,
	BATTLEGROUND = 0x5C,
	BATTLEGROUND_LEADER = 0x5D,
};

enum PlayerChatTag : std::uint8_t {
	TAG_NONE = 0,
	TAG_AFK = 1,
	TAG_DND = 2,
	TAG_GM = 3,
};

struct MessageChat final {
	ChatType type;
	std::uint32_t language;
	std::string message;
	std::uint64_t speech_bubble_attr;
	std::uint64_t chat_name_attr;
	std::string channel_name;
	std::uint32_t player_rank;
	std::uint64_t player_guid;
	PlayerChatTag player_tag;

	StreamResult read_from_stream(le_stream auto& stream) try {
		stream >> type;
		stream >> language;

		if(type == SAY || type == YELL || type == PARTY) {
			stream >> speech_bubble_attr;
			stream >> chat_name_attr;
		} else if(type == CHANNEL) {
			stream >> spark::io::prefixed_null_terminated<std::string, std::uint32_t>(channel_name);
			stream >> player_rank;
			stream >> player_guid;
		}

		stream >> spark::io::prefixed_null_terminated<std::string, std::uint32_t>(message);
		stream >> player_tag;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(le_stream auto& stream) const try {
		stream << type;
		stream << language;

		if(type == SAY || type == YELL || type == PARTY) {
			stream << speech_bubble_attr;
			stream << chat_name_attr;
		} else if(type == WHISPER) {
			//stream << destination;
		} else if(type == CHANNEL) {
			stream << spark::io::null_terminated(channel_name);
			stream << player_rank;
			stream << player_guid;
		} else {
			stream << player_guid;
		}

		stream << spark::io::prefixed_null_terminated<const std::string>(message);
		stream << player_tag;
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // server, protocol, ember
