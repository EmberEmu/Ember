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

struct ChannelNotify final {
	enum ChatNotify : std::uint8_t {
		/// %s joined channel,
		JOINED_NOTICE = 0x00,
		/// %s left channel,
		LEFT_NOTICE = 0x01,
		/// Joined Channel: %s -- You joine,
		YOU_JOINED_NOTICE = 0x02,
		/// Left Channel: %s -- You lef,
		YOU_LEFT_NOTICE = 0x03,
		/// Wrong password for %s,
		WRONG_PASSWORD_NOTICE = 0x04,
		/// Not on channel %s,
		NOT_MEMBER_NOTICE = 0x05,
		/// Not a moderator of %s,
		NOT_MODERATOR_NOTICE = 0x06,
		/// %s Password changed by %s,
		PASSWORD_CHANGED_NOTICE = 0x07,
		/// %s Owner changed to %s,
		OWNER_CHANGED_NOTICE = 0x08,
		/// %s Player %s was not found,
		PLAYER_NOT_FOUND_NOTICE = 0x09,
		/// %s You are not the channel owner,
		NOT_OWNER_NOTICE = 0x0A,
		/// %s Channel owner is %s,
		CHANNEL_OWNER_NOTICE = 0x0B,
		MODE_CHANGE_NOTICE = 0x0C,
		/// %s Channel announcements enabled by %s,
		ANNOUNCEMENTS_ON_NOTICE = 0x0D,
		/// %s Channel announcements disabled by %s,
		ANNOUNCEMENTS_OFF_NOTICE = 0x0E,
		/// %s Channel moderation enabled by %s,
		MODERATION_ON_NOTICE = 0x0F,
		/// %s Channel moderation disabled by %s,
		MODERATION_OFF_NOTICE = 0x10,
		/// %s You do not have permission to speak,
		MUTED_NOTICE = 0x11,
		/// %s Player %s kicked by %s,
		PLAYER_KICKED_NOTICE = 0x12,
		/// %s You are banned from that channel,
		BANNED_NOTICE = 0x13,
		/// %s Player %s banned by %s,
		PLAYER_BANNED_NOTICE = 0x14,
		/// %s Player %s unbanned by %s,
		PLAYER_UNBANNED_NOTICE = 0x15,
		/// %s Player %s is not banned,
		PLAYER_NOT_BANNED_NOTICE = 0x16,
		/// %s Player %s is already on the channel,
		PLAYER_ALREADY_MEMBER_NOTICE = 0x17,
		/// %2$s has invited you to join the channel '%1$s',
		INVITE_NOTICE = 0x18,
		/// Target is in the wrong alliance for %s,
		INVITE_WRONG_FACTION_NOTICE = 0x19,
		/// Wrong alliance for %s,
		WRONG_FACTION_NOTICE = 0x1A,
		/// Invalid channel nam,
		INVALID_NAME_NOTICE = 0x1B,
		/// %s is not moderate,
		NOT_MODERATED_NOTICE = 0x1C,
		/// %s You invited %s to join the channe,
		PLAYER_INVITED_NOTICE = 0x1D,
		/// %s %s has been banned,
		PLAYER_INVITE_BANNED_NOTICE = 0x1E,
		/// %s The number of messages that can be sent to this channel is limited, please wait to send another message,
		THROTTLED_NOTICE = 0x1F
	};

	ChatNotify type;
	std::string name;

	StreamResult read_from_stream(le_stream auto& stream) try {
		stream >> type;
		stream >> spark::io::null_terminated(name);
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}

	StreamResult write_to_stream(le_stream auto& stream) const try {
		stream << type;
		stream << spark::io::null_terminated(name);

		if(type == YOU_JOINED_NOTICE) {
			stream << std::int32_t(0); // unused
			stream << std::int32_t(0); // channel instance display

			/*
			 * One of the following strings can be sent as an optional additional message.
			 * Technically, any message can be sent but it'll trigger a Lua error if there
			 * isn't a matching CHAT_*_NOTICE formatter string. Additionally, the formatter
			 * must take the same number of arguments as CHAT_JOIN_NOTIFY_NOTICE.
			 * SUSPENDED
             * WRONG_FACTION
             * WRONG_PASSWORD
             * YOU_JOINED
             * YOU_LEFT
             * BANNED
             * INVALID_NAME
             * INVITE_WRONG_FACTION
             * MUTED
             * NOT_MEMBER
             * NOT_MODERATED
             * NOT_MODERATOR
             * NOT_OWNER
			 */ 
			std::string_view message("MUTED");
			stream << spark::io::null_terminated(message);
		} else {
			stream << std::uint32_t(0);
		}
		return stream? StreamResult::success : StreamResult::failed;
	} catch(const std::exception&) {
		return StreamResult::caught_exception;
	}
};

} // server, protocol, ember
