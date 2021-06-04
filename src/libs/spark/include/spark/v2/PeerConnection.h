/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Dispatcher.h>
#include <array>
#include <concepts>
#include <utility>
#include <cstdint>
#include <memory>
#include <vector>
#include <cstdio>
#include <iostream> // todo, temp

namespace ember::spark::v2 {

template<std::movable Socket>
class PeerConnection final {
	static constexpr auto HEADER_SIZE = 4u;
	static constexpr auto MAX_MESSAGE_SIZE = 512 * 1024u;
	static constexpr auto MAX_PAYLOAD_SIZE = MAX_MESSAGE_SIZE - HEADER_SIZE;

	Dispatcher& dispatcher_;
	Socket socket_;
	std::array<std::uint8_t, MAX_MESSAGE_SIZE> buff_;

	std::size_t write_offset_ = 0;
	std::size_t payload_len_ = 0;
	bool out_of_data_ = false;

	enum class ReadState {
		HEADER, BODY
	} state_ = ReadState::HEADER;

	void receive() {
		if(!socket_.is_open()) {
			return;
		}

		auto buff_data = buff_.data() + write_offset_;
		const auto buff_capacity = buff_.size() - write_offset_;

		socket_.async_receive(boost::asio::buffer(buff_data, buff_capacity),
			[this](boost::system::error_code ec, std::size_t size) {
				if(!ec) {
					write_offset_ += size;
					out_of_data_ = false;
					handle_packet();
					receive();
				} else if(ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		);
	}

	void handle_packet() {
		do {
			if(state_ == ReadState::HEADER) {
				handle_header();
			}

			if(state_ == ReadState::BODY) {
				handle_body();
			}
		} while(!out_of_data_);
	}

	void handle_header() {
		if(write_offset_ < HEADER_SIZE) {
			out_of_data_ = true;
			return;
		}

		payload_len_ = *reinterpret_cast<std::uint32_t*>(buff_.data());
		state_ = ReadState::BODY;
	}

	void handle_body() {
		const auto payload_read = write_offset_ - HEADER_SIZE;

		if(payload_len_ > MAX_PAYLOAD_SIZE) {
			write_offset_ = 0;
			state_ = ReadState::HEADER;
			return;
		} else if(payload_len_ < payload_read) {
			out_of_data_ = true;
			return;
		}

		if(payload_read >= payload_len_) {
			dispatcher_.receive(); // todo
		}

		// todo, copy any unhandled data to the start of the buffer
		std::memmove(buff_.data(), buff_.data(), 0);
		state_ = ReadState::HEADER;
	}

public:
	PeerConnection(Dispatcher& dispatcher, Socket socket)
		: dispatcher_(dispatcher), socket_(std::move(socket)) {
		receive();
	}

	PeerConnection(PeerConnection&&) = default;

	void send(std::unique_ptr<std::vector<std::uint8_t>> buffer) {
		if(!socket_.is_open()) {
			return;
		}
		
		const auto buff_raw = buffer.get();

		socket_.async_send(buff_raw, 
			[this, buffer = std::move(buffer)](boost::system::error_code ec, std::size_t size) {
				if(!ec) {

				} else if(ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		);
	}

	void close_session() {
		// todo
	}
};

} // spark, ember