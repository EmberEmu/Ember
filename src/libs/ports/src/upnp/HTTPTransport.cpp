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
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <stdexcept>

namespace ember::ports::upnp {

HTTPTransport::HTTPTransport(ba::io_context& ctx, const std::string& bind)
	: socket_(ctx, ba::ip::tcp::endpoint(ba::ip::address::from_string(bind), 0)),
	resolver_(ctx),
	timeout_(ctx) {
	socket_.set_option(boost::asio::ip::tcp::no_delay(true));
	buffer_.resize(INITIAL_BUFFER_SIZE);
}

HTTPTransport::~HTTPTransport() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.close(ec);
}

ba::awaitable<void> HTTPTransport::connect(std::string_view host, const std::uint16_t port) {
	auto results = co_await resolver_.async_resolve(std::string(host),
													std::to_string(port),
													ba::deferred);
	co_await boost::asio::async_connect(socket_, results.begin(), results.end(), ba::deferred);
}

ba::awaitable<void> HTTPTransport::send(std::shared_ptr<std::vector<std::uint8_t>> message) {
	auto buffer = boost::asio::buffer(message->data(), message->size());
	co_await ba::async_write(socket_, buffer, as_tuple(ba::deferred));
}

ba::awaitable<void> HTTPTransport::send(std::vector<std::uint8_t> message) {
	auto data = std::make_shared<std::vector<std::uint8_t>>(std::move(message));
	auto buffer = boost::asio::buffer(data->data(), data->size());
	co_await ba::async_write(socket_, buffer, as_tuple(ba::deferred));
}

auto HTTPTransport::receive_http_response() -> ba::awaitable<Response> {
	start_timer();

	std::size_t total_read = 0;

	// Check header completion
	do {
		total_read += co_await read(total_read);
	} while(!http_headers_completion(total_read));
	
	std::string_view view(buffer_.data(), total_read);
	constexpr std::string_view header_delim("\r\n\r\n");
	const auto headers_end = view.find(header_delim);
	std::string_view header_view { buffer_.data(), headers_end };
	HTTPHeader header;

	if(!parse_http_header(header_view, header)) {
		throw std::invalid_argument("Bad HTTP headers");
	}

	// Check body completion (if present)
	while(http_body_completion(header, total_read)) {
		total_read += co_await read(total_read);
	}

	timeout_.cancel();
	co_return std::make_pair(header, std::span(buffer_.cbegin(), total_read));
}

std::size_t HTTPTransport::http_body_completion(const HTTPHeader& header,
                                                const std::size_t total_read) {
	std::string_view view(buffer_.data(), total_read);
	constexpr std::string_view header_delim("\r\n\r\n");
	const auto headers_end = view.find(header_delim);
	const auto length_field = header.fields.find("Content-Length");

	if(length_field != header.fields.end()) {
		const std::size_t length = sv_to_ll(length_field->second);
		const std::size_t expected_size = length + headers_end + header_delim.size();
		const std::size_t remaining = expected_size - total_read;

		if(remaining > expected_size) {
			return 0;
		}

		return remaining;
	}
	
	return 0;
}

bool HTTPTransport::http_headers_completion(const std::size_t total_read) {
	HTTPHeader header;
	std::string_view view(buffer_.data(), total_read);
	constexpr std::string_view header_delim("\r\n\r\n");
	const auto headers_end = view.find(header_delim);

	if(headers_end == std::string_view::npos) {
		return false;
	}

	return true;
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

ba::awaitable<std::size_t> HTTPTransport::read(const std::size_t offset) {
	if(!buffer_resize(offset)) {
		throw std::length_error("Unable to resize HTTP response buffer");
	}

	const auto buffer = ba::buffer(buffer_.data() + offset, buffer_.size() - offset);
	auto size = co_await socket_.async_receive(buffer, ba::deferred);
	co_return size;
}

void HTTPTransport::start_timer() {
	timeout_.expires_after(READ_TIMEOUT);
	timeout_.async_wait([&](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		if(timeout_.expiry() <= std::chrono::steady_clock::now()) {
			throw std::runtime_error("Did not receive HTTP response in a timely manner");
		}
	});
}

void HTTPTransport::close() {
	socket_.close();
}

ba::ip::tcp::endpoint HTTPTransport::local_endpoint() const {
	return socket_.local_endpoint();
}

bool HTTPTransport::is_open() const {
	return socket_.is_open();
}

} // upnp, ports, ember