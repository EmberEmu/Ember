/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/Packet.h>
#include <protocol/StreamResult.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <shared/utility/UTF8String.h>
#include <boost/container/small_vector.hpp>
#include <gsl/narrow>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>
#include <zlib.h>

namespace ember::protocol::client {

struct AuthSession final {
	static const std::size_t DIGEST_LENGTH = 20;

	struct AddonData {
		std::string name;
		std::uint8_t key_version;
		std::uint32_t crc;
		std::uint32_t update_url_crc;
	};

	std::array<std::uint8_t, DIGEST_LENGTH> digest;
	std::uint32_t seed;
	std::uint32_t id;
	std::uint32_t security;
	std::uint32_t server_id;
	std::uint32_t build;
	std::uint8_t locale;
	utf8_string username;
	boost::container::small_vector<AddonData, 64> addons;

	StreamResult read_from_stream(le_stream auto& stream) {
		const auto initial_stream_size = stream.size();

		stream >> build;
		stream >> server_id;
		stream >> spark::io::null_terminated(username);
		stream >> seed;
		stream >> digest;
		
		// handle compressed addon data
		std::uint32_t decompressed_size;
		stream >> decompressed_size;

		if(!stream.read_limit()) {
			return StreamResult::bad_field_size;
		}

		// calculate how many bytes are left in this message
		const auto remaining = stream.read_limit() - stream.total_read();
		const uLongf compressed_size = gsl::narrow<uLongf>(remaining);

		if(decompressed_size > 0xFFFFF) {
			return StreamResult::bad_field_size;
		}
		
		boost::container::small_vector<std::uint8_t, 512> source(
			remaining, boost::container::default_init
		);

		boost::container::small_vector<std::uint8_t, 4096> dest(
			decompressed_size, boost::container::default_init
		);

		uLongf dest_len = decompressed_size;
		stream.get(source.data(), remaining);
		
		auto ret = uncompress(dest.data(), &dest_len, source.data(), compressed_size);

		if(ret != Z_OK) {
			return StreamResult::decompression_failed;
		}

		spark::io::BufferAdaptor buffer(dest);
		spark::io::BinaryStream addon_stream(buffer, spark::io::no_throw, spark::io::endian::little);

		while(!addon_stream.empty()) {
			AddonData data;
			addon_stream >> spark::io::null_terminated(data.name);
			addon_stream >> data.key_version;
			addon_stream >> data.crc;
			addon_stream >> data.update_url_crc;

			if(!addon_stream) {
				return StreamResult::stream_error;
			}

			addons.emplace_back(std::move(data));
		}		

		return stream? StreamResult::success : StreamResult::stream_error;
	}

	StreamResult write_to_stream(le_stream auto& stream) const {
		stream << build;
		stream << server_id;
		stream << spark::io::null_terminated(username);
		stream << seed;
		stream << digest;
		return stream? StreamResult::success : StreamResult::stream_error;
	}
};

} // client, protocol, ember

namespace ember::protocol {

using cmsg_auth_session = ClientPacket<ClientOpcode::cmsg_auth_session, client::AuthSession>;

} // protocol, ember