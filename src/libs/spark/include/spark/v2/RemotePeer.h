/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Spark_generated.h"
#include <spark/v2/Connection.h>
#include <spark/v2/Channel.h>
#include <spark/v2/Handler.h>
#include <spark/v2/SharedDefs.h>
#include <boost/asio/awaitable.hpp>
#include <logger/Logging.h>
#include <array>
#include <concepts>
#include <memory>
#include <string>
#include <utility>

namespace ember::spark::v2 {

class HandlerRegistry;

class RemotePeer final {
	enum class State {
		HELLO, NEGOTIATING, DISPATCHING
	} state_ = State::HELLO;

	std::shared_ptr<Connection> conn_;
	std::string banner_;
	HandlerRegistry& registry_;
	std::array<std::shared_ptr<Channel>, 256> channels_{};
	log::Logger* log_;

	void send(std::unique_ptr<Message> msg);
	Handler* find_handler(const core::OpenChannel* msg);
	std::uint8_t next_empty_channel();

	void handle_control_message(std::span<const std::uint8_t> data);
	void handle_channel_message(const MessageHeader& header, std::span<const std::uint8_t> data);
	void handle_open_channel(const core::OpenChannel* msg);
	void handle_open_channel_response(const core::OpenChannelResponse* msg);
	void handle_close_channel(const core::CloseChannel* msg);

	void open_channel_response(core::Result result, std::uint8_t id, std::uint8_t requested);
	void send_close_channel(std::uint8_t id);
	void send_open_channel(const std::string& name, const std::string& type, std::uint8_t id);
	void receive(std::span<const std::uint8_t> data);

public:
	RemotePeer(Connection connection, std::string banner, HandlerRegistry& registry, log::Logger* log);
	~RemotePeer();

	void open_channel(const std::string& type, Handler* handler);
	void remove_handler(Handler* handler);
	void start();
};

} // spark, ember