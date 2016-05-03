/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientConnection.h"
#include <game_protocol/Packets.h>
#include <spark/Spark.h>

namespace ember {

void ClientConnection::send_auth_challenge() {
	auto packet = std::make_unique<protocol::SMSG_AUTH_CHALLENGE>();
	auto chain = std::make_shared<spark::ChainedBuffer<1024>>();
	spark::BinaryStream stream(*chain);
	packet->write_to_stream(stream);
	write_chain(chain);
}

bool ClientConnection::handle_packet(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	bool error;
	boost::optional<protocol::PacketHandle> packet = handler_.try_deserialise(buffer, &error);

	if(packet) {

	}

	return error;
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

// todo - post through to socket's io_service & remove strand
void ClientConnection::write(std::shared_ptr<Packet> packet) {
	auto self(shared_from_this());

	if(!socket_.is_open()) {
		return;
	}

	socket_.async_send(boost::asio::buffer(*packet),
		strand_.wrap(create_alloc_handler(allocator_,
			[this, packet, self](boost::system::error_code ec, std::size_t) {
				if(ec && ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
	)));
}

// todo - post through to socket's io_service & remove strand
template<std::size_t BlockSize>
void ClientConnection::write_chain(std::shared_ptr<spark::ChainedBuffer<BlockSize>> chain) {
	auto self(shared_from_this());

	if(!socket_.is_open()) {
		return;
	}

	socket_.async_send(*chain,
		strand_.wrap(create_alloc_handler(allocator_,
		[this, self, chain](boost::system::error_code ec, std::size_t size) {
			chain->skip(size);

			if(ec && ec != boost::asio::error::operation_aborted) {
				close_session();
			} else if(!ec && chain->size()) {
				write_chain(chain); 
			}
		}
	)));
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
		strand_.wrap(create_alloc_handler(allocator_,
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
	)));
}

void ClientConnection::stop() {
	auto self(shared_from_this());

	strand_.post([this, self] {
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