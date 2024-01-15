/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Attributes.h>
#include <stun/Protocol.h>
#include <stun/Transport.h>
#include <stun/Logging.h>
#include <spark/buffers/BinaryInStream.h>
#include <boost/asio/io_context.hpp>
#include <array>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <variant>
#include <cstdint>
#include <cstddef>

namespace ember::stun {

enum Protocol {
	UDP, TCP, TLS_TCP
};

enum RFCMode {
	RFC5389, RFC3489
};

namespace detail {

struct Transaction {
	// :grimacing:
	using VariantPromise = std::variant <
		std::promise<attributes::MappedAddress>,
		std::promise<std::vector<attributes::Attribute>>
	>;

	std::array<std::uint32_t, 4> tx_id;
	std::uint8_t retries;
	std::chrono::milliseconds retry_timeout;
	VariantPromise promise;
};

} // detail

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
	LogCB logger_ = [](Verbosity, LogReason){};
	Verbosity verbosity_ = Verbosity::STUN_LOG_TRIVIAL;

	// todo, thread safety (worker thread may access, figure this out)
	std::unordered_map<std::size_t, detail::Transaction> transactions_;

	std::size_t header_hash(const Header& header);
	void handle_response(std::vector<std::uint8_t> buffer);
	std::vector<attributes::Attribute>
		handle_attributes(spark::BinaryInStream& stream, detail::Transaction& tx);
	void binding_request(detail::Transaction::VariantPromise vp);

	// attribute handlers
	void handle_error_response(spark::BinaryInStream& stream);

	std::optional<attributes::XorMappedAddress>
		handle_xor_mapped_address_opt(spark::BinaryInStream& stream, const detail::Transaction& tx);
	std::optional<attributes::XorMappedAddress>
		handle_xor_mapped_address(spark::BinaryInStream& stream, const detail::Transaction& tx);
	std::optional<attributes::MappedAddress>
		handle_mapped_address(spark::BinaryInStream& stream);

public:
	Client(RFCMode mode = RFCMode::RFC5389);
	~Client();

	void log_callback(LogCB callback, Verbosity verbosity);
	void connect(const std::string& host, std::uint16_t port, const Protocol protocol);
	std::future<attributes::MappedAddress> external_address();
	void software();
};

} // stun, ember