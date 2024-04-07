/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/PeerHandler.h>
#include <spark/v2/PeerConnection.h>
#include <spark/v2/Channel.h>
#include <spark/v2/Dispatcher.h>
#include <spark/v2/SharedDefs.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/container/flat_map.hpp>
#include <logger/Logging.h>
#include <concepts>
#include <string>
#include <utility>

namespace ember::spark::v2 {

class HandlerRegistry;

class RemotePeer final : public Dispatcher {
	enum class State {
		HELLO, NEGOTIATING, DISPATCHING
	} state_ = State::HELLO;

	PeerHandler handler_;
	PeerConnection conn_;
	HandlerRegistry& registry_;
	log::Logger* log_;
	boost::container::flat_map<std::uint8_t, Channel> channels_;

	template<typename T>
	void finish(T& payload, Message& msg);
	void send(std::unique_ptr<Message> msg);
	void write_header(Message& msg);

public:
	RemotePeer(boost::asio::ip::tcp::socket socket, HandlerRegistry& registry, log::Logger* log);

	boost::asio::awaitable<void> send_banner(const std::string& banner);
	boost::asio::awaitable<std::string> receive_banner();

	void send();
	void receive(std::span<const std::uint8_t> data) override;
};

} // spark, ember