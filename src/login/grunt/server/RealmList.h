/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../ResultCodes.h"
#include <shared/Realm.h>
#include <boost/assert.hpp>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

/*
 * The parsing for this packet is pretty verbose but I'm assuming that the data
 * from the server may be invalid (unlikely but better safe than sorry)
 */
namespace ember { namespace grunt { namespace server {

namespace be = boost::endian;

class RealmList final : public Packet {
	static const std::size_t WIRE_LENGTH = 3; // header size 
	static const std::size_t ZERO_REALM_BODY_WIRE_LENGTH = 7; // unknown through to unknown2
	static const std::size_t MINIMUM_REALM_ENTRY_WIRE_LENGTH = 14; // if the realm had a single-byte name
	static const std::size_t MAX_ALLOWED_LENGTH = 4096;
	static const std::size_t MAX_REALM_ENTRIES = 255;

	State state_ = State::INITIAL;
	std::uint16_t size;
	std::uint8_t realm_count;

	std::uint16_t calculate_size() {
		std::size_t packet_size = ZERO_REALM_BODY_WIRE_LENGTH + 
		                          (MINIMUM_REALM_ENTRY_WIRE_LENGTH * realms.size());

		for(auto& entry : realms) {
			packet_size += entry.realm.name.size();
			packet_size += entry.realm.ip.size();
		}

		BOOST_ASSERT_MSG(packet_size <= std::numeric_limits<std::uint16_t>::max(),
		                 "Realm packet too large!");

		return static_cast<std::uint16_t>(packet_size);
	}

	void read_size(spark::BinaryStream& stream) {
		stream >> opcode;
		stream >> size;
		be::little_to_native_inplace(size);

		if(!size || size > MAX_ALLOWED_LENGTH) {
			throw bad_packet("Realm packet size was too large");
		}

		state_ = State::CALL_AGAIN;
	}

	void parse_body(spark::BinaryStream& stream) {
		if(stream.size() < size) {
			return;
		}

		if(stream.size() < ZERO_REALM_BODY_WIRE_LENGTH) {
			throw bad_packet("Realm packet < ZERO_REALM_BODY_WIRE_LENGTH");
		}

		stream >> unknown;
		be::little_to_native_inplace(unknown);
		stream >> realm_count;

		for(; realm_count; --realm_count) {
			if(stream.size() < MINIMUM_REALM_ENTRY_WIRE_LENGTH) {
				throw bad_packet("Realm packet < MINIMUM_REALM_ENTRY_WIRE_LENGTH");
			}

			Realm realm;
			std::uint8_t num_chars;

			stream >> realm.icon;
			stream >> realm.flags;
			stream >> realm.name;

			if(!stream.size()) {
				throw bad_packet("Unexpected end of realm packet (size zero after parsing name)");
			}

			stream >> realm.ip;

			if(stream.size() < ZERO_REALM_BODY_WIRE_LENGTH) {
				throw bad_packet("Unexpected end of realm packet (not enough bytes to finish realm entry)");
			}

			stream >> realm.population;
			stream >> num_chars;
			stream >> realm.timezone;
			stream.skip(1); // unknown byte, just skip it

			be::little_to_native_inplace(realm.icon);
			be::little_to_native_inplace(realm.population);

			realms.emplace_back(RealmListEntry{ realm, num_chars });
		}

		if(!stream.size()) {
			throw bad_packet("Unexpected end of realm packet (final byte missing)");
		}

		stream >> unknown2;
		be::little_to_native_inplace(unknown2);

		state_ = State::DONE;
	}

public:
	struct RealmListEntry {
		Realm realm;
		std::uint8_t characters;
	};

	Opcode opcode = Opcode::CMD_REALM_LIST;
	std::uint32_t unknown = 0;
	std::vector<RealmListEntry> realms;
	std::uint16_t unknown2 = 5;

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		switch(state_) {
			case State::INITIAL:
				read_size(stream); // intentional fall-through
			case State::CALL_AGAIN:
				parse_body(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) override {
		if(realms.size() > MAX_REALM_ENTRIES) {
			throw bad_packet("Attempted to send too many realm list entries!");
		}

		stream << opcode;
		stream << be::native_to_little(calculate_size());
		stream << be::native_to_little(unknown);
		stream << static_cast<std::uint8_t>(realms.size());

		for(auto& entry : realms) {
			auto& realm = entry.realm;
			stream << be::native_to_little(realm.icon);
			stream << realm.flags;
			stream << realm.name;
			stream << realm.ip;
			stream << be::native_to_little(realm.population);
			stream << entry.characters;
			stream << realm.timezone;
			stream << std::uint8_t(0); // unknown
		}

		stream << be::native_to_little(unknown2);
	}
};

}}} // server, grunt, ember