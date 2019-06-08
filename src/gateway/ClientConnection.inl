/*
 * Copyright (c) 2018 - 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BinaryStream.h>
#include <gsl/gsl_util>
#include <algorithm>

template<typename PacketT>
void ClientConnection::send(const PacketT& packet) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << remote_address() << " <- "
		<< protocol::to_string(packet.opcode) << LOG_ASYNC;

	spark::BinaryStream stream(*outbound_back_);
	stream << typename PacketT::SizeType{} << typename PacketT::OpcodeType{} << packet;

	const auto written = stream.total_write();
	auto size = gsl::narrow<typename PacketT::SizeType>(written - sizeof(typename PacketT::SizeType));
	auto opcode = packet.opcode;

	if(authenticated_) {
		crypto_.encrypt(size);
		crypto_.encrypt(opcode);
	}

	stream.write_seek(spark::SeekDir::SD_START);
	stream << size << opcode;
	stream.write_seek(spark::SeekDir::SD_FORWARD, written - PacketT::HEADER_WIRE_SIZE);

	if(!write_in_progress_) {
		write_in_progress_ = true;
		std::swap(outbound_front_, outbound_back_);
		write();
	}

	if(packet_logger_) {
		packet_logger_->log(packet, PacketDirection::OUTBOUND);
	}

	++stats_.messages_out;
}