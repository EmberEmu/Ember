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
#include "../ResultCodes.h"
#include <botan/bigint.h>
#include <boost/assert.hpp>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

class LoginProof final : public Packet {
	static const std::size_t header_length = 2;
	static const std::size_t body_length = 24;
	static const std::size_t proof_length = 20;

	State state_ = State::initial;

	void read_head(PacketStream& stream) {
		stream >> opcode;
		stream >> result;
	}

	void read_body(PacketStream& stream) {
		// no need to keep reading - the other fields aren't set
		if(result != grunt::Result::success) {
			state_ = State::done;
			return;
		}

		// must be CMD_AUTH_LOGIN_PROOF, so read the rest of the packet
		if(stream.size() < body_length) {
			state_ = State::call_again;
			return;
		}
		
		std::array<std::uint8_t, proof_length> m2_buff;
		stream >> m2_buff;
		std::ranges::reverse(m2_buff);
		M2 = Botan::BigInt(m2_buff);

		stream >> survey_id;
	}

public:
	LoginProof() : Packet(Opcode::cmd_auth_logon_proof) {}

	Result result;
	Botan::BigInt M2;
	std::uint32_t survey_id = 0;

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < header_length) {
			return State::call_again;
		}

		switch(state_) {
			case State::initial:
				read_head(stream);
				[[fallthrough]];
			case State::call_again:
				read_body(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}
		
		return stream? state_ = State::done : state_ = State::err_stream_err;
	}

	State write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << result;

		// no need to stream the rest of the members
		if(result != grunt::Result::success && result != grunt::Result::success_survey) {
			return stream? State::done : State::err_stream_err;
		}

		std::array<std::uint8_t, proof_length> bytes{};
		M2.serialize_to(bytes);
		stream.put(bytes.rbegin(), bytes.rend());
		stream << survey_id;

		return stream? State::done : State::err_stream_err;
	}
};

} // server, grunt, ember