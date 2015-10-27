/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include "../PacketBase.h"
#include "../Exceptions.h"
#include "../ResultCodes.h"
#include <spark/Buffer.h>
#include <spark/BinaryStream.h>
#include <string>
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class LoginChallenge final : public PacketBase {
	const std::size_t MAX_USERNAME_LEN = 11;
	State state_ = State::INITIAL;

public:
	static const std::size_t WIRE_LENGTH = 34;

	Opcode opcode = Opcode::CMSG_LOGIN_CHALLENGE; // todo
	ResultCode error = ResultCode::SUCCESS; // todo
	std::uint16_t size;
	std::uint8_t game[4];
	std::uint8_t major;
	std::uint8_t minor;
	std::uint8_t patch;
	std::uint16_t build;
	std::uint8_t platform[4];
	std::uint8_t os[4];
	std::uint8_t country[4];
	std::uint32_t timezone_bias;
	std::uint32_t ip;
	std::uint8_t username_len;
	std::string username;

	void deserialise_body(spark::Buffer& buffer) {
		spark::BinaryStream stream(buffer);
		//stream >> opcode; // todo
		buffer.skip(1); // todo
		//stream >> error; // todo
		buffer.skip(1); // todo
		stream >> size;
		stream >> game;
		stream >> major;
		stream >> minor;
		stream >> patch;
		stream >> build;
		stream >> platform;
		stream >> os;
		stream >> country;
		stream >> timezone_bias;
		stream >> ip;
		stream >> username_len;

		if(username_len >= MAX_USERNAME_LEN) {
			throw bad_packet("Username length was too long!");
		}
	}

	void deserialise_username(spark::Buffer& buffer) {
		spark::BinaryStream stream(buffer);

		// does the buffer hold enough bytes to complete the username?
		if(buffer.size() >= username_len) {
			stream.get(username, username_len);
			state_ = State::DONE;
		} else {
			state_ = State::CALL_AGAIN;
		}
	}

	State deserialise(spark::Buffer& buffer) override {
		if(state_ == State::INITIAL) {
			deserialise_body(buffer);
			deserialise_username(buffer);
		} else {
			deserialise_username(buffer);
		}

		return state_;
	}
};

}}} // client, grunt, ember