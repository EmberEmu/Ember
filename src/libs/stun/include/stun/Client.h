/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Transport.h>
#include <spark/buffers/BinaryInStream.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <future>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <cstdint>

namespace ember::stun {

enum Protocol {
	UDP, TCP, TLS_TCP
};

enum RFCMode {
	RFC5389, RFC3489
};

struct Request {
	std::uint8_t transaction_id[12];
	std::uint8_t retries;
	std::chrono::milliseconds retry_timeout;
	std::promise<std::string> promise;
};

class Client {
	enum class State {
		INITIAL, CONNECTING, CONNECTED, DISCONNECTED
	} state_ = State::INITIAL;

	boost::asio::io_context ctx_;
	std::jthread worker_;
	std::vector<std::shared_ptr<boost::asio::io_context::work>> work_;

	std::unique_ptr<Transport> transport_;
	bool connected_ = false;
	RFCMode mode_;
	std::random_device rd_;
	std::mt19937 mt_;

	std::unordered_map<int, Request> requests_;
	std::promise<std::string> result; // temp todo

	void rng_store(std::uint64_t rng, std::span<std::uint8_t> buffer);
	void handle_response(std::vector<std::uint8_t> buffer);
	void handle_attributes(spark::BinaryInStream& stream);
	void handle_xor_mapped_address(spark::BinaryInStream& stream, std::uint16_t length);
	void handle_mapped_address(spark::BinaryInStream& stream, std::uint16_t length);
	void xor_buffer(std::span<std::uint8_t> buffer, const std::vector<std::uint8_t>& key);

public:
	Client(RFCMode mode = RFCMode::RFC5389);
	~Client();
	void connect(const std::string& host, std::uint16_t port, const Protocol protocol);
	std::future<std::string> mapped_address();
	void software();
};

} // stun, ember