/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientStates.h"
#include "states/Authentication.h"
#include "states/CharacterList.h"
#include "states/WorldForwarder.h"
#include <game_protocol/Packet.h>
#include <game_protocol/Packets.h> // todo, fdecls
#include <game_protocol/PacketHeaders.h> // todo, remove
#include <spark/Buffer.h>
#include <logger/Logging.h>
#include <botan/bigint.h>
#include <cstdint>
#include <vector>

namespace ember {

class ClientConnection;

class ClientHandler final {
	ClientConnection& connection_;

	protocol::ClientHeader* header_;
	log::Logger* logger_;

	// no base to avoid vtable lookups for every packet
	ClientState state_;
	Authentication auth_state_;
	CharacterList list_state_;
	WorldForwarder world_state_;

	std::uint32_t auth_seed_;
	std::string account_name_;

	// state handlers

	void handle_character_list(spark::Buffer& buffer);
	void handle_in_world(spark::Buffer& buffer);

	// opcode handlers
	void handle_ping(spark::Buffer& buffer);
	void handle_char_enum(spark::Buffer& buffer);
	void handle_char_create(spark::Buffer& buffer);
	void handle_char_delete(spark::Buffer& buffer);
	//void handle_char_rename(spark::Buffer& buffer);

	// misc
	void send_character_list(std::vector<Character> characters);
	void send_character_create();
	void send_character_delete();
	void send_character_list_fail();
	void handle_login(spark::Buffer& buffer);

public:
	explicit ClientHandler(ClientConnection& connection, log::Logger* logger)
		: connection_(connection), logger_(logger), state_(ClientState::AUTHENTICATING),
	      auth_state_(*this, connection, logger), list_state_(*this), world_state_(*this) { }
	~ClientHandler();

	void update_state(ClientState state) { state_ = state; }
	bool packet_deserialise(protocol::Packet& packet, spark::Buffer& stream);
	void handle_packet(protocol::ClientHeader header, spark::Buffer& buffer);
	void start(); // todo - remove?
};

} // ember