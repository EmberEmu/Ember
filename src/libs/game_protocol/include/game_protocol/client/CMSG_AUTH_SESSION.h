/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <spark/buffers/ChainedBuffer.h>
#include <logger/Logging.h>
#include <botan/botan.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <zlib.h>

namespace ember { namespace protocol {

namespace be = boost::endian;

class CMSG_AUTH_SESSION final : public Packet {
	static const std::size_t DIGEST_LENGTH = 20;

	State state_ = State::INITIAL;
	std::uint16_t size_ = 0;

public:
	struct AddonData {
		std::string name;
		std::uint8_t key_version;
		std::uint32_t crc;
		std::uint32_t update_url_crc;
	};

	Botan::SecureVector<Botan::byte> digest;
	std::uint32_t seed;
	std::uint32_t id;
	std::uint32_t security;
	std::uint32_t unk1;
	std::uint32_t build;
	std::uint8_t locale;
	std::string username;
	std::vector<AddonData> addons;

	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		auto initial_stream_size = stream.size();

		stream >> build;
		stream >> unk1;
		stream >> username;
		stream >> seed;

		digest.resize(DIGEST_LENGTH);
		stream.get(digest.begin(), DIGEST_LENGTH);
		
		// handle compressed addon data
		be::little_uint32_t decompressed_size;
		stream >> decompressed_size;

		// calculate how much of the remaining stream data belongs to this message
		// we don't want to consume bytes belongining to any messages that follow
		auto remaining = size_ - (initial_stream_size - stream.size());
		uLongf compressed_size = static_cast<uLongf>(remaining);

		if(decompressed_size > 0xFFFFF) {
			LOG_DEBUG_GLOB << "Rejecting compressed addon data for being too large "  << LOG_ASYNC;
			return (state_ = State::ERRORED);
		}
		
		std::vector<std::uint8_t> source(remaining);
		std::vector<std::uint8_t> dest(decompressed_size);
		uLongf dest_len = decompressed_size;
		uLongf source_len = remaining;
		stream.get(source.data(), source_len);
		
		auto ret = uncompress(dest.data(), &dest_len, source.data(), compressed_size);

		if(ret != Z_OK) {
			LOG_DEBUG_GLOB << "Decompression of addon data failed with code " << ret << LOG_ASYNC;
			return (state_ = State::ERRORED);
		}
		
		// not overly efficient
		spark::ChainedBuffer<1024> buffer;
		buffer.write(dest.data(), dest.size());

		spark::SafeBinaryStream addon_stream(buffer);

		while(!addon_stream.empty()) {
			AddonData data;
			addon_stream >> data.name;
			addon_stream >> data.key_version;
			addon_stream >> data.crc;
			addon_stream >> data.update_url_crc;

			be::little_to_native_inplace(data.crc);
			be::little_to_native_inplace(data.update_url_crc);

			addons.emplace_back(std::move(data));
		}

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

	void set_size(std::uint16_t size) {
		size_ = size;
	}
};

}} // protocol, ember