/*
 * Copyright (c) 2016 - 2026 Ember
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

namespace ember::realm {

struct PlayerLogin final : Event {
	explicit PlayerLogin(std::uint64_t character_id)
		: Event{ EventType::player_login },
		  character_id_(character_id) { }

	const std::uint64_t character_id_;
};

struct QueuePosition final : Event {
	explicit QueuePosition(std::size_t position)	
		: Event { EventType::queue_update_position },
	      position(position) { }

	std::size_t position;
};

struct AccountIDResponse final : Event {
	AccountIDResponse(rpc::Account::Status status, std::uint32_t id)
		: Event { EventType::account_id_response },
	      status(status),
		  account_id(id) { }

	rpc::Account::Status status;
	std::uint32_t account_id;
};

struct SessionKeyResponse final : Event {
	SessionKeyResponse(rpc::Account::Status status, Botan::BigInt key)
		: Event { EventType::session_key_response },
	      status(status),
		  key(std::move(key)) { }

	rpc::Account::Status status;
	Botan::BigInt key;
};

struct CharEnumResponse final : Event {
	CharEnumResponse(rpc::Character::Status status, std::vector<Character> characters)
		: Event{ EventType::char_enum_response },
		  status(status),
		  characters(std::move(characters)) { }

	rpc::Character::Status status;
	std::vector<Character> characters;
};

struct CharCreateResponse final : Event {
	explicit CharCreateResponse(protocol::Result result)
		: Event { EventType::char_create_response },
		  result(result) { }

	protocol::Result result;
};

struct CharDeleteResponse final : Event {
	explicit CharDeleteResponse(protocol::Result result)
		: Event{ EventType::char_delete_response },
		  result(result) { }

	protocol::Result result;
};

struct CharRenameResponse final : Event {
	CharRenameResponse(protocol::Result result, std::uint64_t id, std::string name)
		: Event { EventType::char_rename_response },
		  result(result),
		  character_id(id),
		  name(std::move(name)) { }

	protocol::Result result;
	std::uint64_t character_id;
	std::string name;
};

struct SystemMessage final : Event {
	enum class Type {
		message,
		notification,
		whisper
	};

	SystemMessage(std::string message, Type type)
		: Event { EventType::system_message }
		, message(std::move(message))
		, type(type) {}

	std::string message;
	Type type;
};

} // realm, ember