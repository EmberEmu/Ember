/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "Account_generated.h"
#include "Character_generated.h"
#include <protocol/ResultCodes.h>
#include <protocol/Packets.h>
#include <shared/database/objects/Character.h>
#include <botan/bigint.h>
#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember {

struct PlayerLogin : Event {
	explicit PlayerLogin(std::uint64_t character_id)
	                     : Event{ EventType::PLAYER_LOGIN },
	                       character_id_(character_id) { }

	const std::uint64_t character_id_;
};

struct QueuePosition : Event {
	explicit QueuePosition(std::size_t position) 
	                       : Event { EventType::QUEUE_UPDATE_POSITION },
	                         position(position) { }

	std::size_t position;
};

struct QueueSuccess : Event {
	explicit QueueSuccess(protocol::CMSG_AUTH_SESSION packet)
	                      : Event { EventType::QUEUE_SUCCESS },
	                        packet(std::move(packet)) { }

	protocol::CMSG_AUTH_SESSION packet;
};

struct AccountIDResponse : Event {
	AccountIDResponse(protocol::CMSG_AUTH_SESSION packet, messaging::account::Status status,
	                  std::uint32_t id)
	                  : Event { EventType::ACCOUNT_ID_RESPONSE },
	                    packet(std::move(packet)), status(status), account_id(id) { }

	protocol::CMSG_AUTH_SESSION packet;
	messaging::account::Status status;
	std::uint32_t account_id;
};

struct SessionKeyResponse : Event {
	SessionKeyResponse(protocol::CMSG_AUTH_SESSION packet, messaging::account::Status status,
	                   Botan::BigInt key)
	                   : Event { EventType::SESSION_KEY_RESPONSE },
	                     packet(std::move(packet)), status(status), key(key) { }

	protocol::CMSG_AUTH_SESSION packet;
	messaging::account::Status status;
	Botan::BigInt key;
};

struct CharEnumResponse : Event {
	CharEnumResponse(messaging::character::Status status, std::vector<Character> characters)
	                 : Event{ EventType::CHAR_ENUM_RESPONSE },
	                   status(status), characters(std::move(characters)) { }

	messaging::character::Status status;
	std::vector<Character> characters;
};

struct CharCreateResponse : Event {
	CharCreateResponse(messaging::character::Status status, protocol::Result result)
	                   : Event { EventType::CHAR_CREATE_RESPONSE },
	                     status(status), result(result) { }

	messaging::character::Status status;
	protocol::Result result;
};

struct CharDeleteResponse : Event {
	CharDeleteResponse(messaging::character::Status status, protocol::Result result)
	                   : Event{ EventType::CHAR_DELETE_RESPONSE },
	                     status(status), result(result) { }

	messaging::character::Status status;
	protocol::Result result;
};

struct CharRenameResponse : Event {
	CharRenameResponse(messaging::character::Status status, protocol::Result result,
	                   std::uint64_t id, std::string name)
	                   : Event { EventType::CHAR_RENAME_RESPONSE },
	                     status(status), result(result), character_id(id),
	                     name(std::move(name)) { }

	messaging::character::Status status;
	protocol::Result result;
	std::uint64_t character_id;
	std::string name;
};

} // ember