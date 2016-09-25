/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include "SessionManager.h"
#include <spark/buffers/BufferSequence.h>

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
			++stats_.messages_in;
			handler_.handle_packet(packet_header_, buffer);
			read_state_ = ReadState::HEADER;
			continue;
		}

		break;
	}
}

void ClientConnection::send(const protocol::ServerPacket& packet) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << remote_address() << " <- "
		<< protocol::to_string(packet.opcode) << LOG_ASYNC;

	spark::Buffer& buffer(outbound_buffer_);
	spark::SafeBinaryStream stream(buffer);
	const std::size_t write_index = buffer.size(); // the current write index

	stream << std::uint16_t(0) << packet.opcode << packet;

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
			
	++stats_.messages_out;
}

void ClientConnection::write() {
	if(!socket_.is_open()) {
		return;
	}

	spark::BufferSequence<OUTBOUND_SIZE> sequence(outbound_buffer_);

	socket_.async_send(sequence, create_alloc_handler(allocator_,
		[this](boost::system::error_code ec, std::size_t size) {
			stats_.bytes_out += size;
			++stats_.packets_out;

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
	auto tail = inbound_buffer_.back();

	// if the buffer chain has no more space left, allocate & attach new node
	if(!tail->free()) {
		tail = inbound_buffer_.allocate();
		inbound_buffer_.push_back(tail);
	}

	socket_.async_receive(boost::asio::buffer(tail->write_data(), tail->free()),
		create_alloc_handler(allocator_,
		[this](boost::system::error_code ec, std::size_t size) {
			if(stopped_) {
				return;
			}

			if(!ec) {
				stats_.bytes_in += size;
				++stats_.packets_in;

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
	auto k_bytes = Botan::BigInt::encode(key);
	crypto_.set_key(k_bytes);
	authenticated_ = true;
}

void ClientConnection::start() {
	stopped_ = false;
	handler_.start();
	read();
}

void ClientConnection::stop() {
	LOG_DEBUG_FILTER(logger_, LF_NETWORK)
		<< "Closing connection to " << remote_address() << LOG_ASYNC;

	handler_.stop();
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
	stopped_ = true;
}

/* 
 * This function should only be used by the handler to allow for the session to be
 * queued for closure after it has returned from processing the current event/packet.
 * Posting rather than dispatching ensures that the object won't be destroyed by the
 * session manager until current processing has finished.
 */
void ClientConnection::close_session() {
	service_.post([this] {
		sessions_.stop(this);
	});
}

/*
 * This function is used by the destructor to ensure that all current processing
 * has finished before it returns. It uses dispatch rather than post to ensure
 * that if the calling thread happens to be the owner of this connection, that
 * it will be closed immediately, 'in line', rather than blocking indefinitely.
 */
void ClientConnection::close_session_sync() {
	service_.dispatch([&] {
		stop();

		std::unique_lock<std::mutex> ul(stop_lock_);
		stop_condvar_.notify_all();
	});
}

std::string ClientConnection::remote_address() {
	return address_;
}

const ConnectionStats& ClientConnection::stats() const {
	return stats_;
}

void ClientConnection::latency(std::size_t latency) {
	stats_.latency = latency;
}

void ClientConnection::compression_level(unsigned int level) {
	// todo
}

ClientConnection::~ClientConnection() {
	if(!stopped_) {
		close_session_sync();

		while(!stopped_) {
			std::unique_lock<std::mutex> guard(stop_lock_);
			stop_condvar_.wait(guard);
		}
	}
}

} // ember