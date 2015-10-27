/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include "../ResultCodes.h"
#include "../../GameVersion.h"
#include <spark/Buffer.h>
#include <spark/BinaryStream.h>
#include <string>
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class LoginChallenge final : public Packet {
	const std::size_t MAX_USERNAME_LEN = 16;

	State state_ = State::INITIAL;

	void deserialise_body(spark::Buffer& buffer) {
		spark::BinaryStream stream(buffer);
		stream >> opcode;
		stream >> error;
		stream >> size;
		stream >> game;
		stream >> version.major;
		stream >> version.minor;
		stream >> version.patch;
		stream >> version.build;
		stream >> platform;
		stream >> os;
		stream >> country;
		stream >> timezone_bias;
		stream >> ip;

		std::uint8_t username_len;
		stream >> username_len;

		if(username_len > MAX_USERNAME_LEN) {
			throw bad_packet("Username length was too long!");
		}

		username.resize(username_len);
	}

	void deserialise_username(spark::Buffer& buffer) {
		spark::BinaryStream stream(buffer);

		// does the buffer hold enough bytes to complete the username?
		if(buffer.size() >= username.size()) {
			stream.get(username, username.size());
			state_ = State::DONE;
		} else {
			state_ = State::CALL_AGAIN;
		}
	}

public:
	enum Game : std::uint32_t {
		WoW = 'WoW'
	};

	enum Platform : std::uint32_t {
		x86 = 'x86'
	};

	enum OperatingSystem : std::uint32_t {
		Windows = 'Win'
	};

	enum Country : std::uint32_t {
		enGB = 'enGB',
		enUS = 'enUS'
	};

	static const std::size_t WIRE_LENGTH = 34;

	Opcode opcode;
	ResultCode error;
	std::uint16_t size;
	Game game;
	GameVersion version;
	Platform platform;
	OperatingSystem os;
	Country country;
	std::uint32_t timezone_bias;
	std::uint32_t ip;
	std::string username;

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