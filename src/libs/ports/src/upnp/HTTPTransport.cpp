/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/HTTPTransport.h>
#include <ports/upnp/HTTPHeaderParser.h>
#include <ports/upnp/Utility.h>

namespace ember::ports::upnp {

HTTPTransport::HTTPTransport(ba::io_context& ctx, const std::string& bind)
	: socket_(ctx, ba::ip::tcp::endpoint(ba::ip::address::from_string(bind), 0)),
	  resolver_(ctx),
	  timeout_(ctx) {
	buffer_.resize(INITIAL_BUFFER_SIZE);
}

HTTPTransport::~HTTPTransport() {
	socket_.close();
}

void HTTPTransport::connect(std::string_view host, const std::uint16_t port, OnConnect&& cb) {
	resolver_.async_resolve(std::string(host), std::to_string(port),
		[&, cb = std::move(cb)](const boost::system::error_code& ec,
		        ba::ip::tcp::resolver::results_type results) mutable {
			if(!ec) {
				do_connect(std::move(results), std::move(cb));
			} else {
				cb(ec);
			}
		}
	);
}

void HTTPTransport::do_connect(ba::ip::tcp::resolver::results_type results, OnConnect&& cb) {
	boost::asio::async_connect(socket_, results.begin(), results.end(),
		[&, cb = std::move(cb), results](const boost::system::error_code& ec,
		                                 ba::ip::tcp::resolver::iterator) {
			cb(ec);
		}
	);
}

void HTTPTransport::do_write() {
	auto data = std::move(queue_.front());
	auto buffer = boost::asio::buffer(data->data(), data->size());
	queue_.pop();

	ba::async_write(socket_, buffer,
		[this, d = std::move(data)](boost::system::error_code ec, std::size_t /*sent*/) {
			if(ec == boost::asio::error::operation_aborted) {
				return;
			} else if(ec) {
				err_cb_(ec);
				return;
			}

			if(!queue_.empty()) {
				do_write();
			} else {
				buffer_.resize(INITIAL_BUFFER_SIZE);
;				read(0);
			}
		}
	);
}

void HTTPTransport::send(std::shared_ptr<std::vector<std::uint8_t>> message) {
	queue_.emplace(std::move(message));

	if(queue_.size() == 1) {
		do_write();
	}
}

void HTTPTransport::send(std::vector<std::uint8_t> message) {
	auto data = std::make_shared<std::vector<std::uint8_t>>(std::move(message));
	send(std::move(data));
}

void HTTPTransport::receive(const std::size_t total_read) {
	HTTPHeader header;
	std::string_view view(buffer_.data(), total_read);
	constexpr std::string_view header_delim("\r\n\r\n");
	const auto headers_end = view.find(header_delim);

	if(headers_end == std::string_view::npos) {
		read(total_read);
		return;
	}

	std::string_view headers_view(buffer_.data(), headers_end);

	if(!parse_http_header(headers_view, header)) {
		err_cb_(ba::error::invalid_argument);
		return;
	}

	const auto length = header.fields.find("Content-Length");

	if(length != header.fields.end()) {
		const auto expected_size = sv_to_int(length->second)
			+ headers_end + header_delim.size();
		const auto remaining = expected_size - total_read;

		if(remaining) {
			read(total_read);
		} else {
			timeout_.cancel();
			rcv_cb_(header, { buffer_.data(), total_read });
		}
	} else {
		timeout_.cancel();
		rcv_cb_(header, { buffer_.data(), total_read });
	}
}

bool HTTPTransport::buffer_resize(const std::size_t offset) {
	if(offset > buffer_.size()) {
		return false;
	}

	const auto free_space = buffer_.size() - offset;

	if(free_space) {
		return true;
	}
	
	if(buffer_.size() == MAX_BUFFER_SIZE) {
		return false;
	}

	auto new_size = buffer_.size() * 2;

	if(new_size > MAX_BUFFER_SIZE) {
		new_size = MAX_BUFFER_SIZE;
	}

	buffer_.resize(new_size);
	return true;
}

void HTTPTransport::read(const std::size_t offset) {
	if(!buffer_resize(offset)) {
		err_cb_(ba::error::message_size);
	}

	const auto buffer = ba::buffer(buffer_.data() + offset, buffer_.size() - offset);
	start_timer();

	socket_.async_receive(buffer,
		[this, offset](boost::system::error_code ec, std::size_t size) {
			if(ec == ba::error::operation_aborted) {
				return;
			}

			if(!ec) {
				receive(size + offset);
 			} else {
				err_cb_(ec);
			}
		}
	);
}

void HTTPTransport::start_timer() {
	timeout_.expires_after(READ_TIMEOUT);
	timeout_.async_wait([&](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		if(timeout_.expiry() <= std::chrono::steady_clock::now()) {
			err_cb_(ba::error::timed_out);
		}
	});
}

void HTTPTransport::close() {
	socket_.close();
}

ba::ip::tcp::endpoint HTTPTransport::local_endpoint() const {
	return socket_.local_endpoint();
}

void HTTPTransport::set_callbacks(OnReceive receive, OnConnectionError error) {
	rcv_cb_ = receive;
	err_cb_ = error;
}

bool HTTPTransport::is_open() const {
	return socket_.is_open();
}

} // upnp, ports, ember