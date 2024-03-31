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
#include <shared/FilterTypes.h>
#include "Spark_generated.h"
#include <iostream>

namespace ba = boost::asio;

namespace ember::spark::v2 {

RemotePeer::RemotePeer(ba::ip::tcp::socket socket, bool initiate, log::Logger* log)
	: handler_(*this), conn_(*this, std::move(socket)), log_(log) {
	if(initiate) {
		initiate_hello();
		state_ = State::NEGOTIATING;
	}
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
	MessageHeader header;
	header.size = msg->fbb.GetSize();
	header.set_alignment(msg->fbb.GetBufferMinAlignment());

	BufferAdaptor adaptor(msg->header);
	BinaryStream stream(adaptor);
	header.write_to_stream(stream);
	conn_.send(std::move(msg));
}

void RemotePeer::initiate_hello() {
	core::HelloT hello;
	hello.description = "Hello, world"; // temp

	auto msg = std::make_unique<Message>();
	finish(hello, *msg);
	send(std::move(msg));
	state_ = State::NEGOTIATING;
}

void RemotePeer::handle_hello(std::span<const std::uint8_t> data) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	auto fb = core::GetHeader(data.data());
	flatbuffers::Verifier verifier(data.data(), data.size());

	auto hello = fb->message_as_Hello();

	if(!hello->Verify(verifier)) {
		LOG_WARN_FILTER(log_, LF_SPARK)
			<< "[spark] Bad HELLO from "
			<< conn_.address()
			<< LOG_ASYNC;
		return; // todo, end session
	}

	if(hello->magic() != 0x454D4252 || hello->protocol_ver() != 0) {
		const auto msg = std::format(
			"[spark] Incompatible remote peer, {} (magic: {}, version: {})",
			conn_.address(), hello->magic(), hello->protocol_ver()
		);

		LOG_WARN_FILTER(log_, LF_SPARK) << msg << LOG_ASYNC;
		return; // todo, end session
	}

	negotiate_protocols();
	state_ = State::NEGOTIATING;
}

void RemotePeer::negotiate_protocols() {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	core::EnumerateT enumerate;
	auto msg = std::make_unique<Message>();
	enumerate.services.emplace_back("Hello"); // temp
	finish(enumerate, *msg);
	send(std::move(msg));
}


void RemotePeer::send() {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;
}

void RemotePeer::handle_negotiation(std::span<const std::uint8_t> data) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	auto fb = core::GetHeader(data.data());
	flatbuffers::Verifier verifier(data.data(), data.size());

	auto enumerate = fb->message_as_Enumerate();

	if(!enumerate->Verify(verifier)) {
		LOG_WARN_FILTER(log_, LF_SPARK)
			<< "[spark] Bad ENUMERATE from "
			<< conn_.address()
			<< LOG_ASYNC;
		return; // todo, end session
	}

	for(const auto service : *enumerate->services()) {
		LOG_DEBUG(log_) << service->c_str() << LOG_ASYNC;
	}
}

void RemotePeer::receive(std::span<const std::uint8_t> data) {
	LOG_TRACE(log_) << __func__ << LOG_ASYNC;

	spark::v2::BufferAdaptor adaptor(data);
	spark::v2::BinaryStream stream(adaptor);

	MessageHeader header;

	if(header.read_from_stream(stream) != MessageHeader::State::OK
	   || header.size <= stream.total_read()) {
		LOG_WARN_FILTER(log_, LF_SPARK)
			<< "[spark] Bad message header from "
			<< conn_.address()
			<< LOG_ASYNC;
		return;
	}

	const auto header_size = stream.total_read();
	std::span flatbuffer(data.data() + header_size, data.size_bytes() - header_size);

	switch(state_) {
		case State::HELLO:
			handle_hello(flatbuffer);
			break;
		case State::NEGOTIATING:
			handle_negotiation(flatbuffer);
			break;
		case State::DISPATCHING:
			break;
	}
}

} // v2, spark, ember