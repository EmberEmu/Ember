/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "NetworkSession.h"
#include <logger/Logging.h>

namespace ember {

void NetworkSession::read() {
	auto self(shared_from_this());
	auto tail = inbound_buffer_.back();

	// if the buffer chain has no more space left, allocate & attach new node
	if(!tail || !tail->free()) {
		tail = inbound_buffer_.allocate();
		inbound_buffer_.push_back(tail);
	}

	set_timer();

	socket_.async_receive(boost::asio::buffer(tail->write_data(), tail->free()), 
		create_alloc_handler(allocator_,
		[this, self](boost::system::error_code ec, std::size_t size) {
			if(stopped_) {
				return;
			}

			timer_.cancel();

			if(!ec) {
				inbound_buffer_.advance_write(size);

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

void NetworkSession::write(WriteCallback cb) {
	auto self(shared_from_this());
	set_timer();

	const spark::io::BufferSequence sequence(*outbound_front_);

	socket_.async_send(sequence, create_alloc_handler(allocator_,
		[this, self, cb = std::move(cb)](boost::system::error_code ec, std::size_t size) mutable {
		outbound_front_->skip(size);

		if(!ec) {
			if(!outbound_front_->empty()) {
				write(std::move(cb)); // entire buffer wasn't sent, hit gather-write limits?
			} else {
				std::swap(outbound_front_, outbound_back_);

				if(!outbound_front_->empty()) {
					write(std::move(cb));
				} else { // all done!
					write_in_progress_ = false;
					
					if(cb) {
						cb();
					}
				}
			}
		} else if(ec != boost::asio::error::operation_aborted) {
			close_session();
		}
	}));
}

void NetworkSession::set_timer() {
	auto self(shared_from_this());

	timer_.expires_from_now(SOCKET_ACTIVITY_TIMEOUT);
	timer_.async_wait([this, self](const boost::system::error_code& ec) {
		timeout(ec);
	});
}

void NetworkSession::timeout(const boost::system::error_code& ec) {
	if(ec || stopped_) { // if ec is set, the timer was aborted (session close / refreshed)
		return;
	}

	LOG_DEBUG_FILTER(logger_, LF_NETWORK)
		<< "Idle timeout triggered on " << remote_address() << LOG_ASYNC;

	close_session();
}

void NetworkSession::stop() {
	auto self(shared_from_this());

	boost::asio::post(socket_.get_executor(), [this, self] {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Closing connection to " << remote_address() << LOG_ASYNC;

		stopped_ = true;
		boost::system::error_code ec; // we don't care about any errors
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
		timer_.cancel();
	});
}

} // ember