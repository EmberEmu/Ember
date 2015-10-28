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
#include <boost/endian/conversion.hpp>
#include <string>
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class LoginChallenge final : public Packet {
	static const std::size_t MAX_USERNAME_LEN = 16;
	static const std::size_t WIRE_LENGTH = 34;

	State state_ = State::INITIAL;

	void read_body(spark::BinaryStream& stream) {
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

		// handle endianness
		boost::endian::little_to_native_inplace(size);
		boost::endian::little_to_native_inplace(game);
		boost::endian::little_to_native_inplace(platform);
		boost::endian::little_to_native_inplace(os);
		boost::endian::little_to_native_inplace(country);
		boost::endian::little_to_native_inplace(timezone_bias);
		boost::endian::little_to_native_inplace(ip);
	}

	void read_username(spark::BinaryStream& stream) {
		// does the stream hold enough bytes to complete the username?
		if(stream.size() >= username.size()) {
			stream.get(username, username.size());
			state_ = State::DONE;
		} else {
			state_ = State::CALL_AGAIN;
		}
	}

public:
	// todo - use constexpr func when switched to VS2015 - this is implementation defined behaviour!
	enum Game : std::uint32_t {
		WoW = 'WoW'
	};

	enum Platform : std::uint32_t {
		x86 = 'x86',
		PPC = 'PPC'  // is this correct?
	};

	enum OperatingSystem : std::uint32_t {
		Windows = 'Win',
		MacOS = 'Mac' // is this correct?
	};

	enum Country : std::uint32_t {
		enGB = 'enGB',
		enUS = 'enUS'
	};

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

	State read_from_stream(spark::BinaryStream& stream) override {
		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		if(state_ == State::INITIAL) {
			read_body(stream);
			read_username(stream);
		} else {
			read_username(stream);
		}

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) {
		// todo
	}
};

}}} // client, grunt, ember