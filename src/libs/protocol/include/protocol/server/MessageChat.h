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
#include <protocol/server/ChatTypes.h>
#include <stdexcept>
#include <cstdint>

namespace ember::protocol::server {

struct MessageChat final {
	ChatType type;
	std::uint32_t language;
	std::string message;
	std::uint64_t speech_bubble_attr;
	std::uint64_t chat_name_attr;
	std::string channel_name;
	std::uint32_t player_rank;
	std::uint64_t player_guid;
	std::string monster_name;
	PlayerChatTag player_tag;

	StreamResult read_from_stream(le_stream auto& stream) {
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
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << type;
		stream << language;

		if(type == MONSTER_WHISPER) {
			stream << monster_name;
			stream << std::uint64_t(0);
		} else if(type == SAY || type == YELL || type == PARTY) {
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
	}
};

} // server, protocol, ember
