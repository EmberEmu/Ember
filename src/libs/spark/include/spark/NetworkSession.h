/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#define FLATBUFFERS_TRACK_VERIFIER_BUFFER_SIZE
#include "Core_generated.h"
#include <spark/MessageHandler.h>
#include <spark/SessionManager.h>
#include <spark/buffers/ChainedBuffer.h>
#include <shared/FilterTypes.h>
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <flatbuffers/flatbuffers.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::spark {

class NetworkSession : public std::enable_shared_from_this<NetworkSession> {
	const std::size_t MAX_MESSAGE_LENGTH = 1024 * 1024;  // 1MB
	const std::size_t DEFAULT_BUFFER_LENGTH = 1024 * 4;  // 4KB
	enum class ReadState { SIZE_PREFIX, PAYLOAD };

	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_;

	ReadState state_;
	messaging::core::Header* header_;
	std::uint32_t message_size_;
	std::vector<std::uint8_t> in_buff_;
	SessionManager& sessions_;
	MessageHandler handler_;
	const std::string remote_;
	log::Logger* logger_; 
	bool stopped_;

	bool process_header() {
		flatbuffers::Verifier verifier(in_buff_.data(), message_size_);
		auto header = flatbuffers::GetRoot<messaging::core::Header>(in_buff_.data());
		
		if(!header->Verify(verifier)) {
			LOG_WARN_FILTER(logger_, LF_SPARK)
				<< "[spark] Bad header from " << remote_host() << LOG_ASYNC;
			return false;
		}

		return true;
	}

	bool adjust_buffer() {
		std::memcpy(&message_size_, in_buff_.data(), sizeof(message_size_));
		boost::endian::little_to_native_inplace(message_size_);
	
		if(message_size_ > MAX_MESSAGE_LENGTH) {
			LOG_WARN_FILTER(logger_, LF_SPARK)
				<< "[spark] Peer at " << remote_host()
				<< " attempted to send a message of "
				<< message_size_ << " bytes" << LOG_ASYNC;
			return false;
		}

		if(message_size_ > in_buff_.size()) {
			in_buff_.resize(message_size_);
		}

		return true;
	}

	void handle_read() {
		switch(state_) {
			case ReadState::SIZE_PREFIX:
				if(adjust_buffer()) {
					state_ = ReadState::PAYLOAD;
				} else {
					close_session();
				}
				break;
			case ReadState::PAYLOAD:
				if(process_header()) {
					if(handler_.handle_message(*this, header_, in_buff_.data(), message_size_)) {
						state_ = ReadState::SIZE_PREFIX;
					} else {
						close_session();
					}
				} else {
					close_session();
				}
				break;
		}

		read();
	}

	void read() {
		if(stopped_) {
			return;
		}

		auto self(shared_from_this());
		const std::uint32_t read_size = state_ == ReadState::SIZE_PREFIX? sizeof(message_size_) : message_size_;

		boost::asio::async_read(socket_, boost::asio::buffer(in_buff_, read_size), strand_.wrap(
			[this, self](boost::system::error_code ec, std::size_t /*size*/) {
				if(ec && ec != boost::asio::error::operation_aborted) {
					close_session();
					return;
				}

				handle_read();
			}
		));
	}

	void stop() {
		LOG_DEBUG_FILTER(logger_, LF_SPARK)
			<< "[spark] Closing connection to " << remote_host() << LOG_ASYNC;

		stopped_ = true;
		boost::system::error_code ec; // we don't care about any errors
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	}

public:
	NetworkSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket, MessageHandler handler,
	               log::Logger* logger)
	               : sessions_(sessions), socket_(std::move(socket)), message_size_(0),
	                 handler_(handler), logger_(logger), stopped_(false),
	                 state_(ReadState::SIZE_PREFIX), in_buff_(DEFAULT_BUFFER_LENGTH),
	                 strand_(socket_.get_io_service()),
	                 remote_(socket_.remote_endpoint().address().to_string()
	                         + ":" + std::to_string(socket_.remote_endpoint().port())) { }

	void start() {
		handler_.start(*this);
		read();
	}

	void close_session() {
		sessions_.stop(shared_from_this());
	}

	std::string remote_host() {
		return remote_;
	}

	void write(std::shared_ptr<flatbuffers::FlatBufferBuilder> fbb) {
		if(!socket_.is_open()) {
			return;
		}

		auto size = static_cast<decltype(message_size_)>(fbb->GetSize());

		if(size > MAX_MESSAGE_LENGTH) {
			LOG_DEBUG_FILTER(logger_, LF_SPARK)
				<< "[spark] Attempted to send a message larger than permitted size ("
				<< MAX_MESSAGE_LENGTH << " bytes)" << LOG_ASYNC;
			return;
		}

		boost::endian::native_to_little_inplace(size);

		// todo - remove this allocation - waiting for a better solution than can be achieved
		// with the current FlatBuffers custom allocator design
		auto size_ptr = std::make_shared<decltype(message_size_)>(size);

		std::array<boost::asio::const_buffer, 2> buffers {
			boost::asio::const_buffer { size_ptr.get(), sizeof(message_size_) },
			boost::asio::const_buffer { fbb->GetBufferPointer(), fbb->GetSize() }
		};

		auto self(shared_from_this());

		socket_.async_send(buffers, strand_.wrap(
			[this, self, fbb, size_ptr](boost::system::error_code ec, std::size_t /*size*/) {
				if(ec && ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		));
	}

	virtual ~NetworkSession() = default;

	friend class SessionManager;
};

} // spark, ember