/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Spark_generated.h"
#include <spark/v2/PeerHandler.h>
#include <spark/v2/PeerConnection.h>
#include <spark/v2/Channel.h>
#include <spark/v2/Handler.h>
#include <spark/v2/SharedDefs.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <logger/Logging.h>
#include <array>
#include <concepts>
#include <string>
#include <utility>

namespace ember::spark::v2 {

class HandlerRegistry;

class RemotePeer final {
	enum class State {
		HELLO, NEGOTIATING, DISPATCHING
	} state_ = State::HELLO;

	PeerConnection conn_;
	HandlerRegistry& registry_;
	log::Logger* log_;
	std::array<Channel, 256> channels_{};

	template<typename T>
	void finish(T& payload, Message& msg);
	void send(std::unique_ptr<Message> msg);
	void write_header(Message& msg);

	void handle_control_message(std::span<const std::uint8_t> data);
	void handle_channel_message(const MessageHeader& header, std::span<const std::uint8_t> data);
	void handle_open_channel(const core::OpenChannel* msg);
	void handle_open_channel_response(const core::OpenChannelResponse* msg);
	void handle_close_channel(const core::CloseChannel* msg);
	void handle_bye(const core::Bye* msg);

	void open_channel_response(core::Result result, std::uint8_t id, std::uint8_t requested);
	std::uint8_t next_empty_channel();
	void send_close_channel(std::uint8_t id);
	void send_open_channel(const std::string& name, std::uint8_t id);

public:
	RemotePeer(boost::asio::ip::tcp::socket socket, HandlerRegistry& registry, log::Logger* log);

	boost::asio::awaitable<void> send_banner(const std::string& banner);
	boost::asio::awaitable<std::string> receive_banner();

	void receive(std::span<const std::uint8_t> data);
	void open_channel(const std::string& name, Handler* handler);
	void start();
};

} // spark, ember