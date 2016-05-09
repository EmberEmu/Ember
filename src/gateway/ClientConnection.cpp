/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include <game_protocol/Packets.h>

#undef ERROR // temp

namespace ember {

void ClientConnection::parse_header(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	// ClientHeader struct is not packed - do not do sizeof(protocol::ClientHeader)
	const std::size_t header_wire_size
		= sizeof(protocol::ClientHeader::size) + sizeof(protocol::ClientHeader::opcode);

	if(buffer.size() < header_wire_size) {
		return;
	}

	if(authenticated_) {
		crypto_.decrypt(buffer, header_wire_size);
	}

	spark::SafeBinaryStream stream(inbound_buffer_);
	stream >> packet_header_.size >> packet_header_.opcode;

	LOG_TRACE_FILTER(logger_, LF_NETWORK) << remote_address() << " -> "
		<< protocol::to_string(packet_header_.opcode) << LOG_ASYNC;

	read_state_ = ReadState::BODY;
}

void ClientConnection::completion_check(spark::Buffer& buffer) {
	if(buffer.size() < packet_header_.size - sizeof(protocol::ClientHeader::opcode)) {
		return;
	}

	read_state_ = ReadState::DONE;
}

void ClientConnection::process_buffered_data(spark::Buffer& buffer) {
	while(!buffer.empty()) {
		if(read_state_ == ReadState::HEADER) {
			parse_header(buffer);
		}

		if(read_state_ == ReadState::BODY) {
			completion_check(buffer);
		}

		if(read_state_ == ReadState::DONE) {
			handler_.handle_packet(packet_header_, buffer);
			read_state_ = ReadState::HEADER;
			continue;
		}

		break;
	}
}

// todo, remove the need for the opcode arguments
void ClientConnection::send(protocol::ServerOpcodes opcode, std::shared_ptr<protocol::Packet> packet) {
	auto self(shared_from_this());

	service_.dispatch([this, self, opcode, packet]() mutable {
		spark::Buffer& buffer(outbound_buffer_);
		spark::SafeBinaryStream stream(buffer);
		const std::size_t write_index = buffer.size(); // the current write index

		stream << std::uint16_t(0) << opcode << *packet;

		// calculate the size of the packet that we just streamed and then update the buffer
		const boost::endian::big_uint16_at final_size =
			static_cast<std::uint16_t>(stream.size() - write_index) - sizeof(protocol::ServerHeader::size);

		// todo - could implement iterators to make this slightly nicer
		buffer[write_index + 0] = final_size.data()[0];
		buffer[write_index + 1] = final_size.data()[1];

		if(authenticated_) {
			const std::size_t header_wire_size =
				sizeof(protocol::ServerHeader::opcode) + sizeof(protocol::ServerHeader::size);
			crypto_.encrypt(buffer, write_index, header_wire_size);
		}

		if(!write_in_progress_) {
			write_in_progress_ = true;
			write();
		}
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
			} else if(!outbound_buffer_.empty()) {
				// data was buffered at some point between the last send and this handler being invoked
				write();
			} else {
				// all done!
				write_in_progress_ = false;
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

	socket_.async_receive(boost::asio::buffer(tail->write_data(), tail->free()),
		create_alloc_handler(allocator_,
		[this, self](boost::system::error_code ec, std::size_t size) {
			if(stopped_) {
				return;
			}

			if(!ec) {
				inbound_buffer_.advance_write_cursor(size);
				process_buffered_data(inbound_buffer_);
				read();
			} else if(ec != boost::asio::error::operation_aborted) {
				close_session();
			}
		}
	));
}

void ClientConnection::set_authenticated(const Botan::BigInt& key) {
	Botan::SecureVector<Botan::byte> k_bytes = Botan::BigInt::encode(key);
	crypto_.set_key(k_bytes);
	authenticated_ = true;
}

void ClientConnection::start() {
	handler_.start();
	read();
}

void ClientConnection::stop() {
	auto self(shared_from_this());

	service_.dispatch([this, self] {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Closing connection to " << remote_address() << LOG_ASYNC;

		stopped_ = true;
		boost::system::error_code ec; // we don't care about any errors
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	});
}

void ClientConnection::close_session() {
	sessions_.stop(shared_from_this());
}

boost::asio::ip::tcp::socket& ClientConnection::socket() {
	return socket_;
}

std::string ClientConnection::remote_address() {
	return socket_.remote_endpoint().address().to_string() + ":" + std::to_string(socket_.remote_endpoint().port());
}

} // ember