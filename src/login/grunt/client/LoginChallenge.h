/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../Magic.h"
#include "../Exceptions.h"
#include "../ResultCodes.h"
#include <shared/game/GameVersion.h>
#include <shared/utility/UTF8String.h>
#include <boost/assert.hpp>
#include <gsl/narrow>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

class LoginChallenge final : public Packet {
	static const std::size_t max_username_len = 16 * 4; // * 4 to account for UTF8
	static const std::size_t header_length = 4;

	State state_ = State::initial;
	std::uint8_t username_len_ = 0;

	void header(PacketStream& stream) {
		if(stream.size() < header_length) {
			return;
		}

		stream >> opcode;
		stream >> protocol_ver;
		stream >> body_size;
	}

	void read_body(PacketStream& stream) {
		if(stream.size() < body_size) {
			state_ = State::call_again;
			return;
		}

		stream >> game;
		stream >> version.major;
		stream >> version.minor;
		stream >> version.patch;
		stream >> version.build;
		stream >> platform;
		stream >> os;
		stream >> locale;
		stream >> timezone_bias;
		stream >> spark::io::endian::be(ip);

		stream >> spark::io::prefixed<std::string, std::uint8_t>(username);

		if(username.size() > max_username_len) {
			throw bad_packet("Username length was too long!");
		}

		state_ = State::done;
	}

public:
	LoginChallenge()
		: Packet(Opcode::cmd_auth_logon_challenge) {}

	const static int challenge_version = 3;
	const static int reconnect_challenge_version = 2;

	std::uint8_t protocol_ver = 0;
	std::uint16_t body_size = 0;
	Game game;
	GameVersion version = {};
	Platform platform;
	System os;
	Locale locale;
	std::int32_t timezone_bias = 0;
	std::uint32_t ip = 0; // todo - apparently flipped with Mac builds (PPC only?)
	utf8_string username;

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		switch(state_) {
			case State::initial:
				header(stream);
				[[fallthrough]];
			case State::call_again:
				read_body(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	void write_to_stream(PacketStream& stream) const override {
		if(username.length() > max_username_len) {
			throw bad_packet("Provided username was too long!");
		}

		const auto start_pos = stream.total_write();
		stream << opcode;
		stream << protocol_ver;
		const auto size_pos = stream.total_write();
		stream << std::uint16_t(0); // size placeholder
		stream << game;
		stream << version.major;
		stream << version.minor;
		stream << version.patch;
		stream << version.build;
		stream << platform;
		stream << os;
		stream << locale;
		stream << timezone_bias;
		stream << spark::io::endian::be(ip);
		stream << spark::io::prefixed<const utf8_string, std::uint8_t>(username);
		const auto end_pos = stream.total_write();
		const auto size = (end_pos - start_pos) - header_length;

		stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, size_pos);
		stream << gsl::narrow<std::uint16_t>(size);
		stream.write_seek(spark::io::StreamSeek::sk_forward, size);
	}
};

using ReconnectChallenge = LoginChallenge;

} // client, grunt, ember