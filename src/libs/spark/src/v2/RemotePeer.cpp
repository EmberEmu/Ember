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
#include <spark/Exception.h>
#include <shared/FilterTypes.h>
#include "Spark_generated.h"

namespace ba = boost::asio;

namespace ember::spark::v2 {

RemotePeer::RemotePeer(ba::ip::tcp::socket socket, HandlerRegistry& registry, log::Logger* log)
	: handler_(*this),
	  registry_(registry),
	  conn_(*this, std::move(socket)),
	  log_(log) {
}

void RemotePeer::write_header(Message& msg) {
	MessageHeader header;
	header.size = msg.fbb.GetSize();
	header.set_alignment(msg.fbb.GetBufferMinAlignment());

	BufferAdaptor adaptor(msg.header);
	BinaryStream stream(adaptor);
	header.write_to_stream(stream);
}

ba::awaitable<void> RemotePeer::send_banner(const std::string& banner) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	core::HelloT hello;
	hello.description = banner;

	Message msg;
	finish(hello, msg);
	write_header(msg);
	co_await conn_.send(msg);
}

ba::awaitable<std::string> RemotePeer::receive_banner() {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	auto msg = co_await conn_.receive_msg();

	spark::v2::BufferAdaptor adaptor(msg);
	spark::v2::BinaryStream stream(adaptor);

	MessageHeader header;

	if(header.read_from_stream(stream) != MessageHeader::State::OK
	   || header.size <= stream.total_read()) {
		throw exception("bad message header");
	}

	const auto header_size = stream.total_read();
	std::span flatbuffer(msg.data() + header_size, msg.size_bytes() - header_size);

	flatbuffers::Verifier verifier(flatbuffer.data(), flatbuffer.size());
	auto fb = core::GetHeader(flatbuffer.data());
	auto hello = fb->message_as_Hello();

	if(!hello->Verify(verifier)) {
		throw exception("bad flatbuffer message");
	}

	co_return hello->description()->str();
}

template<typename T>
void RemotePeer::finish(T& payload, Message& msg) {
	core::HeaderT header_t;
	core::MessageUnion mu;
	mu.Set(payload);
	header_t.message = mu;
	msg.fbb.Finish(core::Header::Pack(msg.fbb, &header_t));
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
}

void RemotePeer::open_channel(const std::string& name, Handler* handler) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	core::OpenChannelT body;
	body.id = next_channel_id_;
	body.service = name;
	++next_channel_id_;

	auto msg = std::make_unique<Message>();
	finish(body, *msg);
	write_header(*msg);
	conn_.send(std::move(msg));
}

void RemotePeer::start() {
	conn_.start();
}

} // v2, spark, ember