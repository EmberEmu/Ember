/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include "../ResultCodes.h"
#include "../../GameVersion.h"
#include <boost/assert.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace client {

namespace be = boost::endian;

class LoginChallenge final : public Packet {
	static const std::size_t MAX_USERNAME_LEN = 16;
	static const std::size_t WIRE_LENGTH = 34;
	static const std::size_t HEADER_LENGTH = 4; // todo - double check

	State state_ = State::INITIAL;

	void read_body(spark::SafeBinaryStream& stream) {
		stream >> opcode;
		stream >> protocol_ver;
		stream.skip(2); // skip the size field - we don't need it
		stream >> game;
		stream >> version.major;
		stream >> version.minor;
		stream >> version.patch;
		stream >> version.build;
		stream >> platform;
		stream >> os;
		stream >> locale;
		stream >> timezone_bias;
		stream >> ip;

		std::uint8_t username_len;
		stream >> username_len;

		if(username_len > MAX_USERNAME_LEN) {
			throw bad_packet("Username length was too long!");
		}

		username.resize(username_len);

		// handle endianness
		be::little_to_native_inplace(game);
		be::little_to_native_inplace(version.build);
		be::little_to_native_inplace(platform);
		be::little_to_native_inplace(os);
		be::little_to_native_inplace(locale);
		be::little_to_native_inplace(timezone_bias);
		be::big_to_native_inplace(ip);
	}

	void read_username(spark::SafeBinaryStream& stream) {
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
		PPC = 'PPC'
	};

	enum OperatingSystem : std::uint32_t {
		Windows = 'Win',
		OSX     = 'OSX'
	};

	enum ClientLocale : std::uint32_t {
		enGB = 'enGB', enUS = 'enUS',
		esMX = 'esMX', ptBR = 'ptBR',
		frFR = 'frFR', deDE = 'deDE',
		esES = 'esES', ptPT = 'ptPT',
		itIT = 'itIT', ruRU = 'ruRU',
		koKR = 'koKR', zhTW = 'zhTW',
		enTW = 'enTW', enCN = 'enCN',
	};

	LoginChallenge() : Packet(Opcode::CMD_AUTH_LOGON_CHALLENGE) {}

	std::uint8_t protocol_ver;
	Game game;
	GameVersion version;
	Platform platform;
	OperatingSystem os;
	ClientLocale locale;
	std::uint32_t timezone_bias;
	std::uint32_t ip; // todo - apparently flipped with Mac builds (PPC only?)
	std::string username;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		switch(state_) {
			case State::INITIAL:
				read_body(stream);
				[[fallthrough]];
			case State::CALL_AGAIN:
				read_username(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		if(username.length() > MAX_USERNAME_LEN) {
			throw bad_packet("Provided username was too long!");
		}

		auto size = static_cast<std::uint16_t>((WIRE_LENGTH + username.length()) - HEADER_LENGTH);

		stream << opcode;
		stream << protocol_ver;
		stream << be::native_to_little(size);
		stream << be::native_to_little(game);
		stream << version.major;
		stream << version.minor;
		stream << version.patch;
		stream << be::native_to_little(version.build);
		stream << be::native_to_little(platform);
		stream << be::native_to_little(os);
		stream << be::native_to_little(locale);
		stream << be::native_to_little(timezone_bias);
		stream << be::native_to_big(ip);
		stream << static_cast<std::uint8_t>(username.length());
		stream.put(username.data(), username.length());
	}
};

typedef LoginChallenge ReconnectChallenge;

}}} // client, grunt, ember