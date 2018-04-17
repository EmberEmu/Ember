/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/ChainedBuffer.h>
#include <spark/buffers/VectorBufferAdaptor.h>
#include <logger/Logging.h>
#include <botan/botan.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <gsl/gsl_util>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <zlib.h>

namespace ember::protocol::cmsg {

namespace be = boost::endian;

class CMSG_AUTH_SESSION final {
	static const std::size_t DIGEST_LENGTH = 20;

	State state_ = State::INITIAL;

public:
	struct AddonData {
		std::string name;
		be::little_uint8_at key_version;
		be::little_uint32_at crc;
		be::little_uint32_at update_url_crc;
	};

	Botan::secure_vector<Botan::byte> digest;
	be::little_uint32_at seed;
	be::little_uint32_at id;
	be::little_uint32_at security;
	be::little_uint32_at unk1;
	be::little_uint32_at build;
	be::little_uint8_at locale;
	std::string username;
	std::vector<AddonData> addons;

	State read_from_stream(spark::BinaryStream& stream) try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		const auto initial_stream_size = stream.size();

		stream >> build;
		stream >> unk1;
		stream >> username;
		stream >> seed;

		digest.resize(DIGEST_LENGTH);
		stream.get(digest.data(), DIGEST_LENGTH);
		
		// handle compressed addon data
		be::little_uint32_t decompressed_size;
		stream >> decompressed_size;

		if(!stream.read_limit()) {
			LOG_ERROR_GLOB << "CMSG_AUTH_SESSION size not specified?" << LOG_ASYNC;
			return (state_ = State::ERRORED);
		}

		// calculate how many bytes are left in this message
		const auto remaining = stream.read_limit() - stream.total_read();
		const uLongf compressed_size = gsl::narrow<uLongf>(remaining);

		if(decompressed_size > 0xFFFFF) {
			LOG_DEBUG_GLOB << "Rejecting compressed addon data for being too large "  << LOG_ASYNC;
			return (state_ = State::ERRORED);
		}
		
		std::vector<std::uint8_t> source(remaining);
		std::vector<std::uint8_t> dest(decompressed_size);
		uLongf dest_len = decompressed_size;
		stream.get(source.data(), remaining);
		
		auto ret = uncompress(dest.data(), &dest_len, source.data(), compressed_size);

		if(ret != Z_OK) {
			LOG_DEBUG_GLOB << "Decompression of addon data failed with code " << ret << LOG_ASYNC;
			return (state_ = State::ERRORED);
		}
	
		spark::VectorBufferAdaptor buffer(dest);
		spark::BinaryStream addon_stream(buffer);

		while(!addon_stream.empty()) {
			AddonData data;
			addon_stream >> data.name;
			addon_stream >> data.key_version;
			addon_stream >> data.crc;
			addon_stream >> data.update_url_crc;
			addons.emplace_back(std::move(data));
		}

		return (state_ = State::DONE);
	} catch(const spark::exception&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::BinaryStream& stream) const {
		stream << build;
		stream << unk1;
		stream << username;
		stream << seed;
		stream.put(digest.data(), digest.size());
	}
};

} // protocol, ember