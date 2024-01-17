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
#include <expected>
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

namespace detail {

struct Transaction {
	// :grimacing:
	using VariantPromise = std::variant <
		std::promise<std::expected<attributes::MappedAddress, Error>>,
		std::promise<std::expected<std::vector<attributes::Attribute>, Error>>
	>;

	std::array<std::uint32_t, 4> tx_id;
	std::uint8_t retries;
	std::chrono::milliseconds retry_timeout;
	VariantPromise promise;
};

} // detail

class Client {
	boost::asio::io_context ctx_;
	std::jthread worker_;
	std::vector<std::shared_ptr<boost::asio::io_context::work>> work_;

	std::unique_ptr<Transport> transport_;
	RFCMode mode_;
	std::random_device rd_;
	std::mt19937 mt_;
	LogCB logger_ = [](Verbosity, Error){};
	Verbosity verbosity_ = Verbosity::STUN_LOG_TRIVIAL;

	// todo, thread safety (worker thread may access, figure this out)
	std::unordered_map<std::size_t, detail::Transaction> transactions_;

	template<typename T> auto extract_ip_pair(spark::BinaryInStream& stream);
	template<typename T> auto extract_ipv4_pair(spark::BinaryInStream& stream);

	void fulfill_promise(detail::Transaction& tx, std::vector<attributes::Attribute> attributes);
	std::size_t header_hash(const Header& header);
	void handle_response(std::vector<std::uint8_t> buffer);
	std::vector<attributes::Attribute>
		handle_attributes(spark::BinaryInStream& stream, detail::Transaction& tx);
	void binding_request(detail::Transaction::VariantPromise vp);

	std::optional<attributes::Attribute> extract_attribute(spark::BinaryInStream& stream,
	                                                       detail::Transaction& tx);
	attributes::XorMappedAddress handle_xor_mapped_address(spark::BinaryInStream& stream,
	                                                       const detail::Transaction& tx);
	attributes::MappedAddress handle_mapped_address(spark::BinaryInStream& stream);

public:
	Client(RFCMode mode = RFCMode::RFC5389);
	~Client();

	void log_callback(LogCB callback, Verbosity verbosity);
	void connect(const std::string& host, std::uint16_t port, const Protocol protocol);
	std::future<std::expected<attributes::MappedAddress, Error>> external_address();
	std::future<std::expected<std::vector<attributes::Attribute>, Error>> binding_request();
	void software();
};

} // stun, ember