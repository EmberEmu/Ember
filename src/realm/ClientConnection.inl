/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logger.h>
#include <spark/buffers/BinaryStream.h>
#include <gsl/narrow>

namespace ember::realm {

template<protocol::is_packet PacketType>
bool ClientConnection::write_packet_stream(const PacketType& packet) {
	using SizeType = typename PacketType::SizeType;

	spark::io::BinaryStream stream(
		*outbound_back_, spark::io::no_throw, spark::io::endian::little
	);

	if(!packet.write_to_stream(stream)) {
		return false;
	}

	const auto written = stream.total_write();

	auto size = spark::io::endian::native_to_big(
		gsl::narrow_cast<SizeType>(written - sizeof(SizeType))
	);

	auto opcode = packet.opcode;

	if(crypt_) [[likely]] {
		crypt_->encrypt(size);
		crypt_->encrypt(opcode);
	}

	stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, 0);
	stream << size << opcode;
	stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, written);
	return !!stream;
}

void ClientConnection::send(const protocol::is_packet auto& packet) {
	// we're in a bad state if writing fails, we can't recover
	if(!write_packet_stream(packet)) [[unlikely]] {
		LOG_WARN(logger_, "Failed to write packet to stream");
		close_session();
		return;
	}

	// the buffer size is dynamic, so ensure it isn't growing too large
	if(outbound_back_->size() > max_outbound_size) {
		LOG_DEBUG(logger_, "Outbound buffer too large, dropping client");
		close_session();
		return;
	}

	// initiate sending if not already in progress and we're not corked
	if(cork_depth_ == 0) {
		flush();
	}

	if(packet_logger_) [[unlikely]] {
		packet_logger_->log(packet, PacketDirection::outbound);
	}

	++stats_.messages_out;
}

} // realm, ember