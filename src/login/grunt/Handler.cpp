/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Handler.h"
#include "Exceptions.h"
#include "Packets.h"
#include <logger/Logger.h>
#include <shared/utility/FormatPacket.h>
#include <boost/assert.hpp>
#include <vector>
#include <cstdint>

namespace ember::grunt {

template<std::derived_from<Packet> T>
void Handler::create_packet() {
	curr_packet_ = &packet_.emplace<T>();
}

void Handler::dump_bad_packet(BufferType& buffer, std::size_t offset) {
	std::size_t valid_bytes = offset - buffer.size();

	PacketStream stream(buffer);
	stream.skip(stream.size()); // discard any remaining data, we don't care about it anymore

	// recombobulate the data by serialising the packet
	curr_packet_->write_to_stream(stream);

	std::vector<std::uint8_t> contig_buff(stream.size());
	stream.get(contig_buff.data(), stream.size());

	auto output = utility::format_packet(contig_buff.data(), contig_buff.size());

	LOG_ERROR(logger_, "Deserialisation error! Triggered by {} bytes. Dumping packet...\n", valid_bytes);
	LOG_ERROR(logger_, output);
}

void Handler::handle_new_packet(BufferType& buffer) {
	using enum Opcode;
	Opcode opcode;
	buffer.copy(&opcode);

	spark::io::endian::little_to_native_inplace(opcode);
	state_ = State::read;

	switch(opcode) {
		case cmd_auth_logon_challenge:
			[[fallthrough]];
		case cmd_auth_reconnect_challenge:
			create_packet<client::LoginChallenge>();
			break;
		case cmd_auth_logon_proof:
			create_packet<client::LoginProof>();
			break;
		case cmd_auth_reconnect_proof:
			create_packet<client::ReconnectProof>();
			break;
		case cmd_survey_result:
			create_packet<client::SurveyResult>();
			break;
		case cmd_realm_list:
			create_packet<client::RequestRealmList>();
			break;
		case cmd_xfer_accept:
			create_packet<client::TransferAccept>();
			break;
		case cmd_xfer_resume:
			create_packet<client::TransferResume>();
			break;
		case cmd_xfer_cancel:
			create_packet<client::TransferCancel>();
			break;
		default:
			state_ = State::read_error;
			curr_packet_ = nullptr;
	}
}

void Handler::handle_read(BufferType& buffer, std::size_t offset) {
	PacketStream stream(buffer);
	const auto state = curr_packet_->read_from_stream(stream);

	switch(state) {
		case Packet::State::done:
			state_ = State::new_packet;
			break;
		case Packet::State::call_again:
			state_ = State::read;
			break;
		case Packet::State::err_stream_err:
			dump_bad_packet(buffer, offset);
			[[fallthrough]];
		default:
			state_ = State::read_error;
	}
}

const std::expected<Packet*, Handler::State> Handler::process_buffer(BufferType& buffer) {
	if(state_ == State::new_packet) {
		handle_new_packet(buffer);
	}

	if(state_ == State::read) {
		handle_read(buffer, buffer.size());
	}

	if(state_ == State::read_error) {
		return std::unexpected(state_);
	}

	return state_ == State::new_packet? curr_packet_ : nullptr;
}

} // grunt, ember