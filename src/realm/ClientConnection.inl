/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BinaryStream.h>
#include <gsl/narrow>

namespace ember::realm {

template<protocol::is_packet<BinaryStream> PacketType>
bool ClientConnection::write_packet_stream(const PacketType& packet) {
	using SizeType = typename PacketType::SizeType;

	spark::io::BinaryStream stream(*outbound_back_, spark::io::endian::little);

	if(!packet.write_to_stream(stream)) {
		return false;
	}

	const auto end_pos = stream.total_write();
	auto size = gsl::narrow<SizeType>(end_pos - sizeof(SizeType));
	auto opcode = packet.opcode;

	spark::io::endian::conditional_reverse_inplace<
		std::endian::native, std::endian::big
	>(size);

	if(crypt_) [[likely]] {
		crypt_->encrypt(size);
		crypt_->encrypt(opcode);
	}

	stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, 0);
	stream << size << opcode;
	stream.write_seek(spark::io::StreamSeek::sk_stream_absolute, end_pos);
	return true;
}

void ClientConnection::send(const protocol::is_packet<BinaryStream> auto& packet) {
	LOG_TRACE_ASYNC(logger_,"{} <- {}", remote_address(), protocol::to_string(packet.opcode));

	if(outbound_back_->size() > max_outbound_size) {
		LOG_DEBUG_ASYNC(logger_, "Outbound buffer too large, dropping client");
		close_session();
		return;
	}

	// we're in a bad state if writing fails, we can't recover
	if(!write_packet_stream(packet)) [[unlikely]] {
		LOG_WARN_ASYNC(logger_, "Failed to write packet to stream");
		close_session();
		return;
	}

	// initiate sending if not already in progress
	if(!write_in_progress_) {
		std::swap(outbound_front_, outbound_back_);
		write();
	}

	if(packet_logger_) [[unlikely]] {
		packet_logger_->log(packet, PacketDirection::outbound);
	}

	++stats_.messages_out;
}

} // realm, ember