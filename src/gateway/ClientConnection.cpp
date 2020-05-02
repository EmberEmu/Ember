/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include "SessionManager.h"
#include "packetlog/FBSink.h"
#include "packetlog/LogSink.h"
#include <protocol/PacketHeaders.h>
#include <spark/buffers/BufferSequence.h>
#include <algorithm>

namespace ember {

void ClientConnection::parse_header(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	if(buffer.size() < protocol::ClientHeader::WIRE_SIZE) {
		return;
	}

	if(authenticated_) {
		crypto_.decrypt(buffer, protocol::ClientHeader::WIRE_SIZE);
	}

	spark::BinaryStream stream(buffer);
	stream >> msg_size_;

	read_state_ = ReadState::BODY;
}

void ClientConnection::completion_check(const spark::Buffer& buffer) {
	if(buffer.size() < msg_size_) {
		return;
	}

	read_state_ = ReadState::DONE;
}

void ClientConnection::dispatch_message(spark::Buffer& buffer) {
	protocol::ClientOpcode opcode;
	buffer.copy(&opcode, sizeof(opcode));

	LOG_TRACE_FILTER(logger_, LF_NETWORK) << remote_address() << " -> "
		<< protocol::to_string(opcode) << LOG_ASYNC;

	handler_.handle_message(buffer, msg_size_);
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

			if(packet_logger_) {
				packet_logger_->log(buffer, msg_size_, PacketDirection::INBOUND);
			}
			
			dispatch_message(buffer);
			read_state_ = ReadState::HEADER;
			continue;
		}

		break;
	}
}

void ClientConnection::write() {
	if(!socket_.is_open()) {
		return;
	}

	const spark::BufferSequence<OUTBOUND_SIZE> sequence(*outbound_front_);

	socket_.async_send(sequence, create_alloc_handler(allocator_,
		[this](boost::system::error_code ec, std::size_t size) {
			stats_.bytes_out += size;
			++stats_.packets_out;

			outbound_front_->skip(size);

			if(!ec) {
				if(!outbound_front_->empty()) {
					write(); // entire buffer wasn't sent, hit gather-write limits?
				} else {
					std::swap(outbound_front_, outbound_back_);

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
	if(!socket_.is_open()) {
		return;
	}

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

void ClientConnection::set_authenticated(const Botan::BigInt& key) {
	auto k_bytes = Botan::BigInt::encode(key);
	crypto_.set_key(std::move(k_bytes));
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

		std::unique_lock<std::mutex> ul(stop_lock_);
		stop_condvar_.notify_all();
	});
}

std::string ClientConnection::remote_address() const {
	return ep_.address().to_string();
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

		while(!stopped_) {
			std::unique_lock<std::mutex> guard(stop_lock_);
			stop_condvar_.wait(guard);
		}
	}
}

/*
 * Closes the socket and then posts a final event that keeps the client alive
 * until all pending handlers are executed with 'operation_aborted'.
 * That's the theory anyway.
 */
void ClientConnection::async_shutdown(std::shared_ptr<ClientConnection> client) {
	client->terminate();

	boost::asio::post(client->socket_.get_executor(), [client]() {
		LOG_TRACE_FILTER_GLOB(LF_NETWORK) << "Handler for " << client->remote_address()
			<< " destroyed" << LOG_ASYNC;
	});
}

void ClientConnection::log_packets(bool enable) {
	// temp
	if(enable) {
		packet_logger_ = std::make_unique<PacketLogger>();
		packet_logger_->add_sink(std::make_unique<FBSink>("temp", "gateway", remote_address()));
		packet_logger_->add_sink(std::make_unique<LogSink>(*logger_, log::Severity::INFO, remote_address()));
	} else {
		packet_logger_.reset();
	}
}

} // ember