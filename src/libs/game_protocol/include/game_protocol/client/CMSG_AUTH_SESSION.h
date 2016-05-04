/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <botan/botan.h>
#include <boost/assert.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember { namespace protocol {

namespace be = boost::endian;

class CMSG_AUTH_SESSION final : public Packet {
	static const std::size_t DIGEST_LENGTH = 20;

	State state_ = State::INITIAL;

public:
	Botan::SecureVector<Botan::byte> digest;
	std::uint32_t seed;
	std::uint32_t id;
	std::uint32_t security;
	std::uint32_t unk1;
	std::uint32_t build;
	std::uint8_t locale;
	std::string username;
	// addon stuff

	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> build;
		stream >> unk1;
		stream >> username;
		stream >> seed;

		digest.resize(DIGEST_LENGTH);
		stream.get(digest.begin(), DIGEST_LENGTH);
		
		// TODO, HANDLE ADDON DATA
		stream.skip(stream.size());

		be::little_to_native_inplace(build);
		be::little_to_native_inplace(unk1);
		be::little_to_native_inplace(seed);

		return (state_ = State::DONE);
	} catch(spark::buffer_underrun&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << be::native_to_little(build);
		stream << be::native_to_little(unk1);
		stream << username;
		stream << be::native_to_little(seed);
		stream.put(digest.begin(), digest.size());
	}

	std::uint16_t size() const override {
		return 0; // todo
	}
};

}} // protocol, ember