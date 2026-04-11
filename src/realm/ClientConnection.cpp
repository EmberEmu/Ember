/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include "ClientHandler.h"
#include "packet_log/FlatbuffersSink.h"
#include "packet_log/LogSink.h"
#include <logger/Logger.h>
#include <protocol/PacketHeaders.h>
#include <spark/buffers/BufferSequence.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <algorithm>

namespace ember::realm {

ClientConnection::ClientConnection(tcp_socket socket, log::Logger& logger)
	: socket_(std::move(socket))
	, remote_ep_(socket_.remote_endpoint())
	, stats_{}
	, msg_size_(0)
	, logger_(logger)
	, read_state_(ReadState::header)
	, stopped_(false)
	, write_in_progress_(false)
	, handler_(nullptr)
	, compression_level_(0)
	, outbound_front_(&outbound_buffers_.front())
	, outbound_back_(&outbound_buffers_.back()) {}

void ClientConnection::parse_header() {
	LOG_TRACE(logger_, log_func);

	if(inbound_buffer_.size() < protocol::ClientHeader::wire_size) {
		return;
	}

	if(crypt_) [[likely]] {
		crypt_->decrypt(inbound_buffer_.read_ptr(), protocol::ClientHeader::wire_size);
	}

	inbound_buffer_.read(&msg_size_);

	spark::io::endian::big_to_native_inplace(msg_size_);

	if(msg_size_ < sizeof(protocol::ClientHeader::OpcodeType)) {
		LOG_DEBUG(logger_, "Invalid message size from {}", remote_address());
		close_session();
		return;
	}

	read_state_ = ReadState::body;
}

void ClientConnection::completion_check() {
	if(inbound_buffer_.size() < msg_size_) {
		return;
	}

	read_state_ = ReadState::done;
}

void ClientConnection::dispatch_message() {
	handler_->handle_message(inbound_buffer_, msg_size_);
}

void ClientConnection::process_buffered_data() {
	do {
		if(read_state_ == ReadState::header) {
			parse_header();
		}

		if(read_state_ == ReadState::body) {
			completion_check();
		}

		if(read_state_ == ReadState::done) {
			++stats_.messages_in;

			if(packet_logger_) [[unlikely]] {
				log_packet();
			}
			
			dispatch_message();
			read_state_ = ReadState::header;
		} else {
			break;
		}
	} while(!inbound_buffer_.empty());
}

void ClientConnection::write() {
	if(!socket_.is_open()) {
		return;
	}

	const spark::io::BufferSequence sequence(*outbound_front_);

	socket_.async_send(sequence, create_alloc_handler(allocator_,
		[this](const boost::system::error_code& ec, const std::size_t size) {
			if(!ec) {
				stats_.bytes_out += size;
				++stats_.async_sends;

				outbound_front_->skip(size);

				if(outbound_front_->empty()) {
					std::swap(outbound_front_, outbound_back_);
	
					if(outbound_front_->empty()) { // all done!
						write_in_progress_ = false;
					} else {
						write();
					}
				} else {
					write(); // entire buffer wasn't sent, hit gather-write limits?
				}
			} else if(ec != boost::asio::error::operation_aborted) {
				close_session();
			}
		}
	));

	write_in_progress_ = true;
}

void ClientConnection::read() {
	if(!socket_.is_open()) {
		return;
	}

	/*
	 * Set a minimum expected transfer amount to mitigate the impact of
	 * any clients sending large messages one byte at a time and wasting
	 * resources on completion checks that can be avoided since we know
	 * how much data is required before we can do anything with the buffer
	 */
	const auto required_space = minimum_transfer();

	// If there's partially processed data in the buffer, we may be
	// able to free space by defragmenting it.
	if(inbound_buffer_.free() < required_space && !inbound_buffer_.defragment()) [[unlikely]] {
		LOG_DEBUG(logger_, "Inbound buffer full, closing {}", remote_address());
		close_session();
		return;
	}

	const auto at_least = boost::asio::transfer_at_least(required_space);

	boost::asio::async_read(socket_, inbound_buffer_.write_span(), at_least,
	                        create_alloc_handler(allocator_,
		[this](const boost::system::error_code& ec, const std::size_t size) {
			if(!ec) {
				stats_.bytes_in += size;
				++stats_.async_receives;

				inbound_buffer_.advance_write(size);
				process_buffered_data();
				read();
			} else if(ec != boost::asio::error::operation_aborted) {
				close_session();
			}
		}
	));
}

void ClientConnection::send(std::span<const std::uint8_t> packet) {
	const auto total_size = outbound_back_->size() + packet.size();

	// the buffer size is dynamic, so ensure it isn't growing too large
	if(total_size > max_outbound_size) {
		LOG_DEBUG(logger_, "Outbound buffer too large, dropping client");
		close_session();
		return;
	}

	spark::io::BinaryStream stream(*outbound_back_);
	stream << packet;

	if(!stream) {
		LOG_WARN(logger_, "Failed to write packet to stream");
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

void ClientConnection::set_key(std::span<const std::uint8_t> key) {
	crypt_ = PacketCrypto<0>(key);
}

void ClientConnection::start() {
	assert(handler_);

	if(stopped_) {
		return;
	}

	read();
}

void ClientConnection::stop() {
	if(stopped_) {
		return;
	}

	stopped_ = true;
	LOG_DEBUG(logger_, "Closing connection to {}", remote_address());
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
}

void ClientConnection::close_session() {
	boost::asio::post(socket_.get_executor(), [&] {
		if(on_disconnect_) {
			on_disconnect_();
		}
	});
}

std::string ClientConnection::remote_address() const {
	return remote_ep_.address().to_string();
}

const ConnectionStats& ClientConnection::stats() const {
	return stats_;
}

void ClientConnection::latency(std::uint32_t latency) {
	stats_.latency = latency;
}

void ClientConnection::compression_level(unsigned int level) {
	compression_level_ = level;
}

void ClientConnection::packet_log_start(std::unique_ptr<PacketLogger> logger) {
	packet_logger_.swap(logger);
}

void ClientConnection::packet_log_stop() {
	packet_logger_.reset();
}

bool ClientConnection::packet_logging() const {
	return !!packet_logger_;
}

void ClientConnection::log_packet() {
	std::span packet(inbound_buffer_.read_ptr(), msg_size_);
	packet_logger_->log(packet, PacketDirection::inbound);
}

/*
 * Returns the minimum number of bytes required before the packet
 * handler will be able to do anything useful with the incoming
 * packet.
 */
inline std::size_t ClientConnection::minimum_transfer() const {
	if(read_state_ == ReadState::header) {
		return protocol::ClientHeader::wire_size;
	} else {
		return msg_size_ - inbound_buffer_.size();
	}
}

void ClientConnection::set_handler(ClientHandler* handler) {
	assert(handler);
	handler_ = handler;
}

void ClientConnection::set_on_disconnect(OnDisconnect on_disconnect) {
	on_disconnect_ = std::move(on_disconnect);
}

ClientConnection::~ClientConnection() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.cancel(ec);
}

} // realm, ember