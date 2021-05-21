/*
 * Copyright (c) 2015 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../ResultCodes.h"
#include <spark/buffers/NullBuffer.h>
#include <shared/Realm.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <gsl/gsl_util>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

namespace be = boost::endian;

class RealmList final : public Packet {
	static const std::size_t WIRE_LENGTH = 3; // header size 

	State state_ = State::INITIAL;
	be::little_uint16_t size = 0;
	be::little_uint8_t realm_count = 0;

	void read_size(spark::BinaryStream& stream) {
		stream >> opcode;
		stream >> size;

		state_ = State::CALL_AGAIN;
	}

	void parse_body(spark::BinaryStream& stream) {
		if(stream.size() < size) {
			return;
		}

		stream >> unknown;
		stream >> realm_count;

		while(realm_count) {
			Realm realm;
			std::uint8_t num_chars;

			stream >> realm.type;
			stream >> realm.flags;
			stream >> realm.name;
			stream >> realm.ip;
			stream >> realm.population;
			stream >> num_chars;

			std::uint8_t realm_cat;
			stream >> realm_cat;
			realm.category = static_cast<decltype(realm.category)>(realm_cat);

			std::uint8_t realm_id;
			stream >> realm_id;
			realm.id = realm_id;

			realms.emplace_back(RealmListEntry{ realm, num_chars });
			--realm_count;
		}

		stream >> unknown2;

		state_ = State::DONE;
	}

public:
	struct RealmListEntry {
		Realm realm;
		std::uint32_t characters;
	};

	RealmList() : Packet(Opcode::CMD_REALM_LIST) {}

	be::little_uint32_t unknown = 0; // appears to be ignored in public clients
	std::vector<RealmListEntry> realms;
	be::little_uint16_t unknown2 = 5; // appears to be ignored in public clients

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		switch(state_) {
			case State::INITIAL:
				read_size(stream);
				[[fallthrough]];
			case State::CALL_AGAIN:
				parse_body(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	std::size_t write_body(spark::BinaryOutStream& stream) const {
		const auto initial_write = stream.total_write();

		stream << unknown;
		stream << gsl::narrow<std::uint8_t>(realms.size());

		for(const auto& entry : realms) {
			auto& realm = entry.realm;
			stream << be::native_to_little(realm.type);
			stream << realm.flags;
			stream << realm.name;
			stream << realm.ip;
			stream << realm.population;
			stream << gsl::narrow_cast<std::uint8_t>(entry.characters);
			stream << gsl::narrow<std::uint8_t>(realm.category);
			stream << gsl::narrow<std::uint8_t>(realm.id);
		}

		stream << unknown2;

		return stream.total_write() - initial_write;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;

		// calculate the size by using a null buffer, then write the message for real
		spark::NullBuffer null_buff;
		spark::BinaryOutStream null_stream(null_buff);

		stream << be::native_to_little(gsl::narrow<std::uint16_t>(write_body(null_stream)));
		write_body(stream);
	}
};

} // server, grunt, ember