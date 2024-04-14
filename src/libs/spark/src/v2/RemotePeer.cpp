/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/RemotePeer.h>
#include <spark/v2/Message.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/HandlerRegistry.h>
#include <spark/v2/Peers.h>
#include <spark/v2/Utility.h>
#include <spark/Exception.h>
#include <shared/FilterTypes.h>
#include <gsl/gsl_util>

namespace ba = boost::asio;

namespace ember::spark::v2 {

RemotePeer::RemotePeer(Connection connection, HandlerRegistry& registry, log::Logger* log)
	: registry_(registry),
	  conn_(std::move(connection)),
	  log_(log) {
}

void RemotePeer::send(std::unique_ptr<Message> msg) {
	write_header(*msg);
	conn_.send(std::move(msg));
}

void RemotePeer::receive(std::span<const std::uint8_t> data) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	spark::v2::BufferAdaptor adaptor(data);
	spark::v2::BinaryStream stream(adaptor);

	MessageHeader header;

	if(header.read_from_stream(stream) != MessageHeader::State::OK
	   || header.size <= stream.total_read()) {
		LOG_WARN_FILTER(log_, LF_SPARK)
			<< "[spark] Bad message from "
			<< conn_.address()
			<< LOG_ASYNC;
		return;
	}

	const auto header_size = stream.total_read();
	std::span flatbuffer(data.data() + header_size, data.size_bytes() - header_size);

	if(header.channel == 0) {
		handle_control_message(flatbuffer);
	} else {
		handle_channel_message(header, flatbuffer);
	}
}

void RemotePeer::handle_open_channel_response(const core::OpenChannelResponse* msg) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	if(msg->result() != core::Result::OK) {
		Channel& channel = channels_[msg->requested_id()];
		LOG_ERROR_FMT(log_, "[spark] Remote peer could not open channel ({}:{})",
			channel.handler()->type(), msg->requested_id());
		channel.reset();
		return;
	}

	// todo, index bounds checks
	auto id = msg->actual_id();
	Channel& channel = channels_[id];

	if(msg->actual_id() != msg->requested_id()) {
		if(channel.state() != Channel::State::EMPTY) {
			LOG_ERROR_FMT(log_, "[spark] Channel open ({}) failed due to ID collision",
				msg->actual_id());
			send_close_channel(msg->actual_id());
			channels_[msg->requested_id()].reset();
			return;
		}

		channels_[msg->actual_id()] = channels_[msg->requested_id()];
		channels_[msg->requested_id()].reset();
	}

	if(channel.state() != Channel::State::HALF_OPEN) {
		send_close_channel(msg->actual_id());
		channels_[msg->actual_id()].reset();
		return;
	}

	channel.state(Channel::State::OPEN);
	LOG_INFO_FMT(log_, "[spark] Remote channel open, {}:{}",
		channel.handler()->name(), msg->actual_id());
}

void RemotePeer::send_close_channel(const std::uint8_t id) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	core::CloseChannelT body {
		.channel = id
	};

	auto msg = std::make_unique<Message>();
	finish(body, *msg);
	write_header(*msg);
	conn_.send(std::move(msg));
}

Handler* RemotePeer::find_handler(const core::OpenChannel* msg) {
	const auto sname = msg->service_name();
	const auto stype = msg->service_type();

	if(sname && stype) {
		return registry_.service(sname->str(), stype->str());
	}

	if(sname) {
		return registry_.service(sname->str());
	}

	if(stype) {
		auto services = registry_.services(stype->str());

		// just use the first matching handler
		if(!services.empty()) {
			return services.front();
		}
;	}

	return nullptr;
}

void RemotePeer::handle_open_channel(const core::OpenChannel* msg) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	auto handler = find_handler(msg);

	if(!handler) {
		LOG_DEBUG_FMT(log_, "[spark] Requested service handler ({}) does not exist",
			msg->service_type()->str());
		open_channel_response(core::Result::ERROR_UNK, 0, msg->id());
		return;
	}

	if(msg->id() == 0 || msg->id() >= channels_.size()) {
		LOG_DEBUG_FMT(log_, "[spark] Bad channel ID ({}) specified", msg->id());
		open_channel_response(core::Result::ERROR_UNK, 0, msg->id());
		return;
	}

	auto id = gsl::narrow<std::uint8_t>(msg->id());
	auto& channel = channels_[id];

	if(channel.state() != Channel::State::EMPTY) {
		if(id = next_empty_channel(); id != 0) {
			channel = channels_[id];
		} else {
			LOG_ERROR_FMT(log_, "[spark] Exhausted channel IDs");
			open_channel_response(core::Result::ERROR_UNK, 0, msg->id());
			return;
		}
	}

	channel.state(Channel::State::OPEN);
	channel.handler(handler);
	open_channel_response(core::Result::OK, id, msg->id());
	LOG_INFO_FMT(log_, "[spark] Remote channel open, {}:{}", handler->name(), id);
}

std::uint8_t RemotePeer::next_empty_channel() {
	// zero is reserved
	for(auto i = 1; i < channels_.size(); ++i) {
		if(channels_[i].state() == Channel::State::EMPTY) {
			return i;
		}
	}

	return 0;
}

void RemotePeer::open_channel_response(const core::Result result,
                                       const std::uint8_t id,
                                       const std::uint8_t requested) {
	core::OpenChannelResponseT response {
		.result = result,
		.requested_id = requested,
		.actual_id = id
	};

	auto msg = std::make_unique<Message>();
	finish(response, *msg);
	write_header(*msg);
	conn_.send(std::move(msg));
}

void RemotePeer::handle_control_message(std::span<const std::uint8_t> data) {
	flatbuffers::Verifier verifier(data.data(), data.size());
	auto fb = core::GetHeader(data.data());
	
	if(!fb->Verify(verifier)) {
		LOG_WARN(log_) << "[spark] Bad Flatbuffer message" << LOG_ASYNC;
		return;
	}

	switch(fb->message_type()) {
		case core::Message::OpenChannel:
			handle_open_channel(fb->message_as_OpenChannel());
			break;
		case core::Message::CloseChannel:
			handle_close_channel(fb->message_as_CloseChannel());
			break;
		case core::Message::OpenChannelResponse:
			handle_open_channel_response(fb->message_as_OpenChannelResponse());
			break;
		default:
			LOG_WARN(log_) << "[spark] Unknown control message type" << LOG_ASYNC;
	}
}

void RemotePeer::handle_close_channel(const core::CloseChannel* msg) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	auto id = gsl::narrow<std::uint8_t>(msg->channel());
	Channel& channel = channels_[id];

	if(channel.state() == Channel::State::EMPTY) {
		LOG_WARN(log_) << "[spark] Request to close empty channel" << LOG_ASYNC;
		return;
	}

	channel.reset();
	LOG_INFO_FMT(log_, "[spark] Closed channel {}, requested by remote peer", id);
}

void RemotePeer::handle_channel_message(const MessageHeader& header,
                                        std::span<const std::uint8_t> data) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	Channel& channel = channels_[header.channel];

	if(channel.state() != Channel::State::OPEN) {
		LOG_WARN_FMT(log_, "[spark] Received message for closed channel, {}", header.channel);
		return;
	}

	channel.message(header, data);
}

void RemotePeer::send_open_channel(const std::string& name,
                                   const std::string& type,
                                   const std::uint8_t id) {
	core::OpenChannelT body {
		.id = id,
		.service_type = type,
		.service_name = name
	};

	auto msg = std::make_unique<Message>();
	finish(body, *msg);
	write_header(*msg);
	conn_.send(std::move(msg));
}

void RemotePeer::open_channel(const std::string& type, Handler* handler) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	const auto id = next_empty_channel();
	LOG_DEBUG_FMT(log_, "[spark] Requesting channel {} for {}", id, type);

	Channel& channel = channels_[id];
	channel.state(Channel::State::HALF_OPEN);
	channel.handler(handler);
	send_open_channel("", type, id);
}

void RemotePeer::start() {
	conn_.start([this](std::span<const std::uint8_t> data) {
		receive(data);
	});
}

// very temporary, not thread-safe etc
void RemotePeer::remove_handler(Handler* handler) {
	for(std::size_t i = 0u; i < channels_.size(); ++i) {
		Channel& channel = channels_[i];

		if(channel.state() == Channel::State::EMPTY) {
			continue;
		}

		if(channel.handler() == handler) {
			send_close_channel(i);
			channel.reset();
		}
	}
}

RemotePeer::~RemotePeer() {
	for(auto& channel : channels_) {
		if(channel.state() == Channel::State::OPEN) {
			// todo, link down all channels
		}
	}
}

} // v2, spark, ember