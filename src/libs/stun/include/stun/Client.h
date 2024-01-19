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
#include <stun/Transaction.h>
#include <stun/Transport.h>
#include <stun/Logging.h>
#include <spark/buffers/BinaryInStream.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <expected>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <cstdint>
#include <cstddef>

namespace ember::stun {

enum Protocol {
	UDP, TCP, TLS_TCP
};

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
	Protocol protocol_;

	int max_udp_retries_ = 0;
	std::chrono::milliseconds udp_initial_timeout_ { 0 };
	std::chrono::milliseconds tcp_timeout_ { 0 };

	// todo, thread safety (worker thread may access, figure this out)
	std::unordered_map<std::size_t, detail::Transaction> transactions_;

	template<typename T> auto extract_ip_pair(spark::BinaryInStream& stream);
	template<typename T> auto extract_ipv4_pair(spark::BinaryInStream& stream);

	// Transaction stuff
	detail::Transaction& start_transaction(detail::Transaction::VariantPromise vp);
	void process_transaction(spark::BinaryInStream& stream, detail::Transaction& tx, MessageType type);
	void complete_transaction(detail::Transaction& tx, std::vector<attributes::Attribute> attributes);
	void abort_transaction(detail::Transaction& tx, Error error);
	void transaction_timer(detail::Transaction& tx);
	std::size_t tx_hash(const TxID& tx_id);
	void set_tcp_timer(detail::Transaction& tx);
	void set_udp_timer(detail::Transaction& tx);

	void handle_response(std::vector<std::uint8_t> buffer);
	std::vector<attributes::Attribute> handle_attributes(spark::BinaryInStream& stream,
	                                                     const detail::Transaction& tx,
	                                                     MessageType type);
	void binding_request(detail::Transaction& tx);

	Error validate_header(const Header& header);
	std::optional<attributes::Attribute> extract_attribute(spark::BinaryInStream& stream,
	                                                       const detail::Transaction& tx,
	                                                       MessageType type);
	bool check_attr_validity(Attributes attr_type, MessageType msg_type, bool required);
	attributes::XorMappedAddress parse_xor_mapped_address(spark::BinaryInStream& stream,
	                                                       const detail::Transaction& tx);
	attributes::MappedAddress parse_mapped_address(spark::BinaryInStream& stream);
	attributes::UnknownAttributes parse_unknown_attributes(spark::BinaryInStream& stream,
	                                                       std::size_t length);
	attributes::ErrorCode parse_error_code(spark::BinaryInStream& stream,
	                                       std::size_t length);
	attributes::MessageIntegrity parse_message_integrity(spark::BinaryInStream& stream);
	attributes::MessageIntegrity256 parse_message_integrity_sha256(spark::BinaryInStream& stream,
	                                                               std::size_t length);
	attributes::Username parse_username(spark::BinaryInStream& stream, std::size_t size);
	attributes::Software parse_software(spark::BinaryInStream& stream, std::size_t size);
	attributes::Fingerprint parse_fingerprint(spark::BinaryInStream& stream);
	void on_connection_error(const boost::system::error_code& error);

public:
	Client(RFCMode mode = RFCMode::RFC5389);
	~Client();

	void log_callback(LogCB callback, Verbosity verbosity);
	void connect(const std::string& host, std::uint16_t port, const Protocol protocol);
	std::future<std::expected<attributes::MappedAddress, Error>> external_address();
	std::future<std::expected<std::vector<attributes::Attribute>, Error>> binding_request();
	void set_udp_initial_timeout(std::chrono::milliseconds timeout);
	void set_max_udp_retries(int retries);
	void set_tcp_timeout(std::chrono::milliseconds timeout);
};

} // stun, ember