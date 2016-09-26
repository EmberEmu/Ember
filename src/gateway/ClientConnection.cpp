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
#include <zlib.h>

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

	spark::Buffer& buffer(*outbound_back_);
	spark::SafeBinaryStream stream(buffer);
	const std::size_t write_index = buffer.size(); // the current write index

	stream << std::uint16_t(0) << packet.opcode << packet;

	// calculate the size of the packet that we just streamed and then update the buffer
	const boost::endian::big_uint16_at final_size =
		static_cast<std::uint16_t>(stream.size() - write_index) - sizeof(protocol::ServerHeader::size);

	buffer[write_index + 0] = final_size.data()[0];
	buffer[write_index + 1] = final_size.data()[1];

	if(authenticated_) {
		const std::size_t header_wire_size =
			sizeof(protocol::ServerHeader::opcode) + sizeof(protocol::ServerHeader::size);
		crypto_.encrypt(buffer, write_index, header_wire_size);
	}

	if(!write_in_progress_) {
		write_in_progress_ = true;
		swap_buffers();
		write();
	}
			
	++stats_.messages_out;
}

void ClientConnection::write() {
	if(!socket_.is_open()) {
		return;
	}

	spark::BufferSequence<OUTBOUND_SIZE> sequence(*outbound_front_);

	socket_.async_send(sequence, create_alloc_handler(allocator_,
		[this](boost::system::error_code ec, std::size_t size) {
			stats_.bytes_out += size;
			++stats_.packets_out;

			outbound_front_->skip(size);

			if(!ec) {
				if(!outbound_front_->empty()) {
					write(); // entire buffer wasn't sent, hit gather-write limits?
				} else {
					swap_buffers();

					if(!outbound_front_->empty()) {
						write();
					} else { // all done!
						write_in_progress_ = false;
					}
				}
			} else if(ec != boost::asio::error::operation_aborted) {
				close_session();
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

void ClientConnection::swap_buffers() {
	if(outbound_front_ == &outbound_buffers_.front()) {
		outbound_front_ = &outbound_buffers_.back();
		outbound_back_ = &outbound_buffers_.front();
	} else {
		outbound_front_ = &outbound_buffers_.front();
		outbound_back_ = &outbound_buffers_.back();
	}
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
	compression_level_ = level;
}

// temp testing function, do not use yet
void ClientConnection::stream_compress(const protocol::ServerPacket& packet) {
	constexpr std::size_t BLOCK_SIZE = 64;
	spark::ChainedBuffer<4096> temp_buffer;
	spark::Buffer& buffer(temp_buffer);
	spark::SafeBinaryStream stream(buffer);
	spark::SafeBinaryStream out_stream(*outbound_back_);
	stream << std::uint16_t(0) << packet.opcode << packet;

	boost::endian::big_int16_t size =
		static_cast<std::uint16_t>(stream.size() - sizeof(protocol::ServerHeader::size));

	temp_buffer[0] = size.data()[0];
	temp_buffer[1] = size.data()[1];

	std::uint8_t in_buff[BLOCK_SIZE];
	std::uint8_t out_buff[BLOCK_SIZE];

	z_stream z_stream{};
	z_stream.next_in = in_buff;
	z_stream.next_out = out_buff;

	deflateInit(&z_stream, compression_level_);

	out_stream << std::uint16_t(0) << std::uint16_t(0x1F6);

	while(!stream.empty()) {
		z_stream.avail_in = stream.size() >= BLOCK_SIZE? BLOCK_SIZE : stream.size();
		stream.get(in_buff, z_stream.avail_in);

		do {
			z_stream.avail_out = BLOCK_SIZE;
			int ret = deflate(&z_stream, Z_NO_FLUSH);

			if(ret != Z_OK) {
				throw std::runtime_error("Zlib failed");
			}

			outbound_back_->write(out_buff, BLOCK_SIZE - z_stream.avail_out);
		} while(z_stream.avail_out == 0);


		outbound_back_->write(out_buff, BLOCK_SIZE - z_stream.avail_out);
	}

	deflateEnd(&z_stream);
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