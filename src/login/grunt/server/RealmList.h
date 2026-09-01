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
#include <shared/Realm.h>
#include <boost/assert.hpp>
#include <boost/container/small_vector.hpp>
#include <gsl/narrow>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

class RealmList final : public Packet {
	static const std::size_t wire_length = 3; // header size 
	static const std::size_t default_realms = 10u;

	State state_ = State::initial;
	std::uint16_t size = 0;
	std::uint8_t realm_count = 0;

	void read_size(PacketStream& stream) {
		stream >> opcode;
		stream >> size;

		state_ = State::call_again;
	}

	void parse_body(PacketStream& stream) {
		if(stream.size() < size) {
			return;
		}

		stream >> unknown;
		stream >> realm_count;
		realms.reserve(realm_count);

		while(realm_count) {
			Realm realm;
			std::uint8_t num_chars;

			stream >> realm.type;
			stream >> realm.flags;
			stream >> spark::io::null_terminated(realm.name);
			stream >> spark::io::null_terminated(realm.address);
			stream >> realm.population;
			stream >> num_chars;

			std::uint8_t realm_cat;
			stream >> realm_cat;
			realm.category = static_cast<decltype(realm.category)>(realm_cat);

			std::uint8_t realm_id;
			stream >> realm_id;
			realm.id = realm_id;

			realms.emplace_back(realm, num_chars);
			--realm_count;
		}

		stream >> unknown2;

		state_ = State::done;
	}

public:
	struct RealmListEntry {
		Realm realm;
		std::uint32_t characters;
	};

	RealmList() : Packet(Opcode::cmd_realm_list) {}

	std::uint32_t unknown = 0; // appears to be ignored in public clients, probably protocol version
	boost::container::small_vector<RealmListEntry, default_realms> realms;
	std::uint16_t unknown2 = 5; // appears to be ignored in public clients

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < wire_length) {
			return State::call_again;
		}

		switch(state_) {
			case State::initial:
				read_size(stream);
				[[fallthrough]];
			case State::call_again:
				parse_body(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return stream? state_ = State::done : state_ = State::err_stream_err;
	}

	std::size_t write_body(PacketStream& stream) const {
		const auto initial_write = stream.total_write();

		stream << unknown;
		stream << gsl::narrow<std::uint8_t>(realms.size());

		for(const auto& entry : realms) {
			auto& realm = entry.realm;
			stream << realm.type;
			stream << realm.flags;
			stream << spark::io::null_terminated(realm.name);
			stream << spark::io::null_terminated(realm.address);
			stream << realm.population;
			stream << gsl::narrow_cast<std::uint8_t>(entry.characters);
			stream << gsl::narrow<std::uint8_t>(realm.category);
			stream << gsl::narrow<std::uint8_t>(realm.id);
		}

		stream << unknown2;

		return stream.total_write() - initial_write;
	}

	State write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << std::uint16_t(0); // write placeholder size
		const auto write_len = write_body(stream);
		const auto end_pos = stream.total_write();

		// update size field
		stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, sizeof(opcode));
		stream << gsl::narrow<std::uint16_t>(write_len);
		stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, end_pos);

		return stream? State::done : State::err_stream_err;
	}
};

} // server, grunt, ember