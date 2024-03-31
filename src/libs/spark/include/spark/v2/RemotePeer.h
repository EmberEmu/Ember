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
#include <boost/container/flat_map.hpp>
#include <logger/Logging.h>
#include <concepts>
#include <utility>

namespace ember::spark::v2 {

class RemotePeer final : public Dispatcher {
	enum class State {
		HELLO, NEGOTIATING, DISPATCHING
	} state_ = State::HELLO;

	PeerHandler handler_;
	PeerConnection conn_;
	log::Logger* log_;
	boost::container::flat_map<std::uint8_t, Channel> channels_;

	template<typename T>
	void finish(T& payload, Message& msg);

	void initiate_hello();
	void negotiate_protocols();
	void handle_hello(std::span<const std::uint8_t> data);
	void handle_negotiation(std::span<const std::uint8_t> data);
	void send(std::unique_ptr<Message> msg);

public:
	RemotePeer(boost::asio::ip::tcp::socket socket, bool initiate, log::Logger* log);

	void send();
	void receive(std::span<const std::uint8_t> data) override;
};

} // spark, ember