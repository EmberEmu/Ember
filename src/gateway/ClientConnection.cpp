/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include <game_protocol/Packets.h>
#include <spark/SafeBinaryStream.h>

namespace ember {

void ClientConnection::send_auth_challenge() {
	auto packet = std::make_shared<protocol::SMSG_AUTH_CHALLENGE>();
	packet->seed = 600; // todo, obviously

	send(protocol::ServerOpcodes::SMSG_AUTH_CHALLENGE, packet);
	state_ = ClientStates::AUTHENTICATING;
}

void ClientConnection::prove_session(const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
	LOG_DEBUG(logger_) << "Received session proof from " << packet.username << LOG_ASYNC;
}

void ClientConnection::handle_authentication(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	if(packet_header_.opcode != protocol::ClientOpcodes::CMSG_AUTH_SESSION) {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Expected CMSG_AUTH_CHALLENGE, dropping "
			<< remote_address() << remote_port() << LOG_ASYNC;
		close_session();
		return;
	}

	spark::SafeBinaryStream stream(buffer);
	protocol::CMSG_AUTH_SESSION packet;

	if(packet.read_from_stream(stream) != protocol::Packet::State::DONE) {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Authentication packet parse failed, disconnecting" << LOG_ASYNC;
		close_session();
		return;
	}

	prove_session(packet);
}

void ClientConnection::dispatch_packet(spark::Buffer& buffer) {
	switch(state_) {
		case ClientStates::AUTHENTICATING:
			handle_authentication(buffer);
			break;
		case ClientStates::IN_QUEUE:
			break;
		case ClientStates::CHARACTER_LIST:
			break;
		case ClientStates::IN_WORLD:
			break;
		default:
			// assert
			break;
	}
}

void ClientConnection::parse_header(spark::Buffer& buffer) {
	if(buffer.size() < sizeof(protocol::ClientHeader)) {
		return;
	}

	buffer.read(&packet_header_.size, sizeof(protocol::ClientHeader::size));
	buffer.read(&packet_header_.opcode, sizeof(protocol::ClientHeader::opcode));

	if(authenticated_) {
		//protocol::decrypt_client_header(header);
	}

	read_state_ = ReadState::BODY;
}

void ClientConnection::completion_check(spark::Buffer& buffer) {
	if(buffer.size() < packet_header_.size - sizeof(protocol::ClientHeader::opcode)) {
		return;
	}

	read_state_ = ReadState::DONE;
}

bool ClientConnection::handle_packet(spark::Buffer& buffer) {
	if(read_state_ == ReadState::HEADER) {
		parse_header(buffer);
	}

	if(read_state_ == ReadState::BODY) {
		completion_check(buffer);
	}

	if(read_state_ == ReadState::DONE) {
		dispatch_packet(buffer);
		read_state_ = ReadState::HEADER;
	}

	return true; // temp
}

void ClientConnection::start() {
	send_auth_challenge();
	read();
}

boost::asio::ip::tcp::socket& ClientConnection::socket() {
	return socket_;
}

std::string ClientConnection::remote_address() {
	return socket_.remote_endpoint().address().to_string();
}

std::uint16_t ClientConnection::remote_port() {
	return socket_.remote_endpoint().port();
}

void ClientConnection::close_session() {
	sessions_.stop(shared_from_this());
}

void ClientConnection::send(protocol::ServerOpcodes opcode, std::shared_ptr<protocol::Packet> packet) {
	auto self(shared_from_this());

	service_.dispatch([this, self, opcode, packet]() {
		spark::SafeBinaryStream stream(outbound_buffer_);
		stream << boost::endian::native_to_big(packet->size() + sizeof(opcode)) << opcode;
		packet->write_to_stream(stream);
		write();
	});
}

void ClientConnection::write() {
	auto self(shared_from_this());

	if(!socket_.is_open()) {
		return;
	}

	socket_.async_send(outbound_buffer_, create_alloc_handler(allocator_,
		[this, self](boost::system::error_code ec, std::size_t size) {
			outbound_buffer_.skip(size);

			if(ec && ec != boost::asio::error::operation_aborted) {
				close_session();
			}
		}
	));
}

void ClientConnection::read() {
	auto self(shared_from_this());
	auto tail = inbound_buffer_.tail();

	// if the buffer chain has no more space left, allocate & attach new node
	if(!tail->free()) {
		tail = inbound_buffer_.allocate();
		inbound_buffer_.attach(tail);
	}

	socket_.async_receive(boost::asio::buffer(tail->data(), tail->free()),
		create_alloc_handler(allocator_,
		[this, self](boost::system::error_code ec, std::size_t size) {
			if(stopped_) {
				return;
			}

			if(!ec) {
				inbound_buffer_.advance_write_cursor(size);

				if(handle_packet(inbound_buffer_)) {
					read();
				} else {
					close_session();
				}
			} else if(ec != boost::asio::error::operation_aborted) {
				close_session();
			}
		}
	));
}

void ClientConnection::stop() {
	auto self(shared_from_this());

	socket_.get_io_service().post([this, self] {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Closing connection to " << remote_address()
			<< ":" << remote_port() << LOG_ASYNC;

		stopped_ = true;
		boost::system::error_code ec; // we don't care about any errors
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	});
}

} // ember