/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Handler.h"
#include "Packets.h"
#include <spark/buffers/pmr/Buffer.h>
#include <boost/assert.hpp>
#include <shared/util/FormatPacket.h>
#include <vector>
#include <cstdint>

namespace ember::grunt {

void Handler::dump_bad_packet(const spark::io::buffer_underrun& e,
                              spark::io::pmr::Buffer& buffer,
                              std::size_t offset) {
	std::size_t valid_bytes = offset - buffer.size();

	spark::io::pmr::BinaryStream stream(buffer);
	stream.skip(stream.size()); // discard any remaining data, we don't care about it anymore

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

void Handler::handle_new_packet(spark::io::pmr::Buffer& buffer) {
	Opcode opcode;
	buffer.copy(&opcode, sizeof(opcode));

	switch(opcode) {
		case Opcode::CMD_AUTH_LOGON_CHALLENGE:
			[[fallthrough]];
		case Opcode::CMD_AUTH_RECONNECT_CHALLENGE:
			packet_ = client::LoginChallenge();
			curr_packet_ = &std::get<client::LoginChallenge>(packet_);
			break;
		case Opcode::CMD_AUTH_LOGON_PROOF:
			packet_ = client::LoginProof();
			curr_packet_ = &std::get<client::LoginProof>(packet_);
			break;
		case Opcode::CMD_AUTH_RECONNECT_PROOF:
			packet_ = client::ReconnectProof();
			curr_packet_ = &std::get<client::ReconnectProof>(packet_);
			break;
		case Opcode::CMD_SURVEY_RESULT:
			packet_ = client::SurveyResult();
			curr_packet_ = &std::get<client::SurveyResult>(packet_);
			break;
		case Opcode::CMD_REALM_LIST:
			packet_ = client::RequestRealmList();
			curr_packet_ = &std::get<client::RequestRealmList>(packet_);
			break;
		case Opcode::CMD_XFER_ACCEPT:
			packet_ = client::TransferAccept();
			curr_packet_ = &std::get<client::TransferAccept>(packet_);
			break;
		case Opcode::CMD_XFER_RESUME:
			packet_ = client::TransferResume();
			curr_packet_ = &std::get<client::TransferResume>(packet_);
			break;
		case Opcode::CMD_XFER_CANCEL:
			packet_ = client::TransferCancel();
			curr_packet_ = &std::get<client::TransferCancel>(packet_);
			break;
		default:
			throw bad_packet("Unknown opcode encountered!");
	}

	state_ = State::READ;
}

void Handler::handle_read(spark::io::pmr::Buffer& buffer, std::size_t offset) try {
	spark::io::pmr::BinaryStream stream(buffer);
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
} catch(const spark::io::buffer_underrun& e) {
	dump_bad_packet(e, buffer, offset);
	throw bad_packet(e.what());
}

const Packet* const Handler::try_deserialise(spark::io::pmr::Buffer& buffer) {
	switch(state_) {
		case State::NEW_PACKET:
			handle_new_packet(buffer);
			[[fallthrough]];
		case State::READ:
			handle_read(buffer, buffer.size());
			break;
	}

	if(state_ == State::NEW_PACKET) {
		return curr_packet_;
	} else {
		return nullptr;
	}
}

} // grunt, ember