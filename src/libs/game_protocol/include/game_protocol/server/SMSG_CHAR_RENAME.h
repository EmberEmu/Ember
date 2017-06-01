/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <boost/endian/conversion.hpp>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::protocol {

namespace be = boost::endian;

class SMSG_CHAR_RENAME final : public ServerPacket {
	State state_ = State::INITIAL;

public:
	SMSG_CHAR_RENAME() : ServerPacket(protocol::ServerOpcodes::SMSG_CHAR_RENAME) { }

	Result result;
	std::uint64_t id;
	std::string name;
	
	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> result;

		if(result == protocol::Result::RESPONSE_SUCCESS) {
			stream >> id;
			stream >> name;
		}

		be::little_to_native_inplace(id);

		return (state_ = State::DONE);
	} catch(spark::buffer_underrun&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << result;

		if(result == protocol::Result::RESPONSE_SUCCESS) {
			stream << be::native_to_little(id);
			stream << name;
		}
	}
};

} // protocol, ember
