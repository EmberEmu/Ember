/*
 * Copyright (c) 2018 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BinaryStream.h>
#include <gsl/narrow>

namespace ember::gateway {

template<protocol::is_packet PacketType>
bool ClientConnection::write_packet_stream(const PacketType& packet) {
	using SizeType = typename PacketType::SizeType;

	spark::io::BinaryStream stream(*outbound_back_);

	if(!packet.write_to_stream(stream)) {
		return false;
	}

	const auto end_pos = stream.total_write();
	auto size = gsl::narrow<SizeType>(end_pos - sizeof(SizeType));
	auto opcode = packet.opcode;

	if(crypt_) [[likely]] {
		crypt_->encrypt(size);
		crypt_->encrypt(opcode);
	}

	stream.write_seek(spark::io::StreamSeek::SK_STREAM_ABSOLUTE, 0);
	stream << size << opcode;
	stream.write_seek(spark::io::StreamSeek::SK_STREAM_ABSOLUTE, end_pos);
	return true;
}

void ClientConnection::send(const protocol::is_packet auto& packet) {
	LOG_TRACE_ASYNC(logger_,"{} <- {}", remote_address(), protocol::to_string(packet.opcode));

	// we're in a bad state if writing fails, we can't recover
	if(!write_packet_stream(packet)) {
		close_session();
		return;
	}

	// initiate sending if not already in progress
	if(!write_in_progress_) {
		std::swap(outbound_front_, outbound_back_);
		write();
	}

	if(packet_logger_) [[unlikely]] {
		packet_logger_->log(packet, PacketDirection::OUTBOUND);
	}

	++stats_.messages_out;
}

} // gateway, ember