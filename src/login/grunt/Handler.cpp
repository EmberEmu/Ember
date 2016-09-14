/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Handler.h"
#include "Packets.h"
#include <spark/Buffer.h>
#include <boost/assert.hpp>
#include <shared/util/FormatPacket.h>
#include <vector>
#include <cstdint>

namespace ember { namespace grunt {

void Handler::dump_bad_packet(const spark::buffer_underrun& e, spark::Buffer& buffer,
                              std::size_t offset) {
	std::size_t valid_bytes = offset - buffer.size();

	spark::BinaryStream stream(buffer);
	stream.clear(); // discard any remaining data, we don't care about it anymore

	// recombobulate the data by serialising the packet
	curr_packet_->write_to_stream(stream);
	std::vector<std::uint8_t> contig_buff(stream.size());
	stream.get(contig_buff.data(), stream.size());

	auto output = util::format_packet(contig_buff.data(), contig_buff.size());

	LOG_ERROR(logger_) << "Buffer stream underrun! \nRead request: "
	                   << e.read_size << " bytes \nBuffer size: " << e.buff_size
	                   << " bytes \nError triggered by first "
	                   << valid_bytes << " bytes \n" << output << LOG_ASYNC;
}

void Handler::handle_new_packet(spark::Buffer& buffer) {
	Opcode opcode;
	buffer.copy(&opcode, sizeof(opcode));

	switch(opcode) {
		case Opcode::CMD_AUTH_LOGIN_CHALLENGE:
			[[fallthrough]];
		case Opcode::CMD_AUTH_RECONNECT_CHALLENGE:
			curr_packet_ = std::make_unique<client::LoginChallenge>();
			break;
		case Opcode::CMD_AUTH_LOGON_PROOF:
			curr_packet_ = std::make_unique<client::LoginProof>();
			break;
		case Opcode::CMD_AUTH_RECONNECT_PROOF:
			curr_packet_ = std::make_unique<client::ReconnectProof>();
			break;
		case Opcode::CMD_SURVEY_RESULT:
			curr_packet_ = std::make_unique<client::SurveyResult>();
			break;
		case Opcode::CMD_REALM_LIST:
			curr_packet_ = std::make_unique<client::RequestRealmList>();
			break;
		case Opcode::CMD_XFER_ACCEPT:
			curr_packet_ = std::make_unique<client::TransferAccept>();
			break;
		case Opcode::CMD_XFER_RESUME:
			curr_packet_ = std::make_unique<client::TransferResume>();
			break;
		case Opcode::CMD_XFER_CANCEL:
			curr_packet_ = std::make_unique<client::TransferCancel>();
			break;
		default:
			throw bad_packet("Unknown opcode encountered!");
	}

	state_ = State::READ;
}

void Handler::handle_read(spark::Buffer& buffer, std::size_t offset) try {
	spark::SafeBinaryStream stream(buffer);
	Packet::State state = curr_packet_->read_from_stream(stream);

	switch(state) {
		case Packet::State::DONE:
			state_ = State::NEW_PACKET;
			break;
		case Packet::State::CALL_AGAIN:
			state_ = State::READ;
			break;
		default:
			BOOST_ASSERT_MSG(false, "Unreachable condition hit!");
	}
} catch(spark::buffer_underrun& e) {
	dump_bad_packet(e, buffer, offset);
	throw bad_packet(e.what());
}

boost::optional<PacketHandle> Handler::try_deserialise(spark::Buffer& buffer) {
	switch(state_) {
		case State::NEW_PACKET:
			handle_new_packet(buffer);
			[[fallthrough]];
		case State::READ:
			handle_read(buffer, buffer.size());
			break;
	}

	if(state_ == State::NEW_PACKET) {
		return std::move(curr_packet_);
	} else {
		return {};
	}
}


}} // grunt, ember