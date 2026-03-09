/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include "SessionManager.h"
#include "packet_log/FBSink.h"
#include "packet_log/LogSink.h"
#include <logger/Logger.h>
#include <protocol/PacketHeaders.h>
#include <spark/buffers/BufferSequence.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <algorithm>

namespace ember::realm {

void ClientConnection::parse_header() {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(inbound_buffer_.size() < protocol::ClientHeader::WIRE_SIZE) {
		return;
	}

	if(crypt_) [[likely]] {
		crypt_->decrypt(inbound_buffer_.read_ptr(), protocol::ClientHeader::WIRE_SIZE);
	}

	inbound_buffer_.read(&msg_size_);

	spark::io::endian::conditional_reverse_inplace<
		std::endian::big, std::endian::native
	>(msg_size_);

	if(msg_size_ < sizeof(protocol::ClientHeader::OpcodeType)) {
		LOG_DEBUG_ASYNC(logger_, "Invalid message size from {}", remote_address());
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
	handler_.handle_message(inbound_buffer_, msg_size_);
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
				std::span packet(inbound_buffer_.read_ptr(), msg_size_);
				packet_logger_->log(packet, PacketDirection::inbound);
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
			stats_.bytes_out += size;
			++stats_.async_sends;

			outbound_front_->skip(size);

			if(!ec) {
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
	const auto min_transfer = minimum_transfer();

	// If there's partially processed data in the buffer, we may be
	// able to free space by defragmenting it.
	if(inbound_buffer_.free() < min_transfer && !inbound_buffer_.defragment()) {
		LOG_DEBUG_ASYNC(logger_, "Inbound buffer full, closing {}", remote_address());
		close_session();
		return;
	}

	const auto transfer_al = boost::asio::transfer_at_least(min_transfer);

	boost::asio::async_read(socket_, inbound_buffer_.write_span(), transfer_al,
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

void ClientConnection::set_key(std::span<const std::uint8_t> key) {
	crypt_ = PacketCrypto(key);
}

void ClientConnection::start() {
	if(stopped_) {
		return;
	}

	// when using DynamicTLSBuffer, we need to ensure the first write
	// (triggered by handler_) is invoked from the service thread
	boost::asio::dispatch(socket_.get_executor(), [&] {
		handler_.start();
		read();
	});
}

void ClientConnection::stop() {
	LOG_DEBUG_ASYNC(logger_, "Closing connection to {}", remote_address());

	handler_.stop();
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
	stopped_ = true;
}

/* 
 * This function must only be called from the io_context responsible for
 * this object. This function will initiate the following:
 * 1) Remove the connection from the session manager - multiple calls to
 *    this function will have no effect. The 'stopping' check is only
 *    to prevent unnecessary locking & lookups.
 * 2) Ownership of the object will be passed to async_shutdown, which in
 *    turn will stop the handler and shutdown/close the socket. This function
 *    blocks until complete.
 * 3) Ownership of the object will be moved into a lambda and one final post
 *    will be made to the associated io_context. Once this final completion handler
 *    is invoked, the object will be destroyed. This should ensure that the
 *    object outlives any aborted completion handlers.
 */
void ClientConnection::close_session() {
	if(stopping_) {
		return;
	}

	stopping_ = true;

	boost::asio::post(socket_.get_executor(), [this] {
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
	boost::asio::dispatch(socket_.get_executor(), [&] {
		stop();

		std::unique_lock ul(stop_lock_);
		stop_condvar_.notify_all();
	});
}

std::string ClientConnection::remote_address() const {
	return remote_ep_.address().to_string();
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

void ClientConnection::terminate() {
	if(!stopped_) {
		close_session_sync();

		std::unique_lock guard(stop_lock_);
		stop_condvar_.wait(guard, [&] { return stopped_.load(); });
	}
}

/*
 * Closes the socket and then posts a final event that keeps the client alive
 * until all pending handlers are executed with 'operation_aborted'.
 * That's the theory anyway.
 */
void ClientConnection::async_shutdown(std::shared_ptr<ClientConnection> client) {
	auto executor = client->socket_.get_executor();
	client->terminate();

	boost::asio::post(executor, [client = std::move(client)]() {
		LOG_TRACE_ASYNC(client->logger_, "Handler for {} destroyed", client->remote_address());
	});
}

void ClientConnection::log_packets(bool enable) {
	// temp - make logger non-ptr and add enable flag?
	if(enable) {
		packet_logger_ = std::make_unique<PacketLogger>();
		packet_logger_->add_sink(std::make_unique<FBSink>("temp", "realm", remote_address()));
		packet_logger_->add_sink(
			std::make_unique<LogSink>(logger_, log::Severity::info, remote_address())
		);
	} else {
		packet_logger_.reset();
	}
}

inline std::size_t ClientConnection::minimum_transfer() const {
	if(read_state_ == ReadState::header) {
		return protocol::ClientHeader::WIRE_SIZE;
	} else {
		return msg_size_ - inbound_buffer_.size();
	}
}

} // realm, ember