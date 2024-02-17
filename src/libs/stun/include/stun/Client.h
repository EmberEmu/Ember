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
#include <stun/Logging.h>
#include <stun/Parser.h>
#include <stun/Util.h>
#include <boost/asio/io_context.hpp>
#include <expected>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <cstdint>
#include <cstddef>

namespace ember::spark {
	class BinaryInStream;
}

namespace ember::stun {

class Transport;

class Client {
	const int TX_RM = 16; // RFC drops magic number, refuses to elaborate
	const int MAX_REDIRECTS = 5;

	std::jthread worker_;
	std::vector<std::shared_ptr<boost::asio::io_context::work>> work_;

	Parser parser_;
	std::unique_ptr<Transport> transport_;
	RFCMode mode_;
	std::random_device rd_;
	std::mt19937 mt_;
	LogCB logger_ = [](Verbosity, Error){};
	Verbosity verbosity_ = Verbosity::STUN_LOG_TRIVIAL;
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> dest_hist_;
	std::string host_;
	std::uint16_t port_;
	std::optional<bool> is_nat_present_;

	// todo, thread safety (worker thread may access, figure this out)
	std::unordered_map<std::size_t, detail::Transaction> transactions_;

	// Transaction stuff
	detail::Transaction& start_transaction(detail::Transaction::VariantPromise vp);
	void complete_transaction(detail::Transaction& tx,
	                          std::vector<attributes::Attribute> attributes);
	void abort_transaction(detail::Transaction& tx, Error error,
	                       attributes::ErrorCode = {}, bool erase = true);
	void transaction_timer(detail::Transaction& tx);
	std::size_t tx_hash(const TxID& tx_id);

	// Message handling stuff
	void handle_message(std::vector<std::uint8_t> buffer);
	void handle_binding_resp(std::vector<attributes::Attribute> attributes,
	                         detail::Transaction& tx);
	void handle_binding_err_resp(const std::vector<attributes::Attribute>& attributes,
	                             detail::Transaction& tx);
	void binding_request(detail::Transaction& tx);
	std::uint32_t calculate_fingerprint(const std::vector<std::uint8_t>& buffer,
	                                    std::size_t offset);
	void process_message(const Header& header, spark::BinaryInStream& stream,
	                     const std::vector<std::uint8_t>& buffer,
	                     detail::Transaction& tx);

	void connect(const std::string& host, std::uint16_t port);
	void set_nat_present(const std::vector<attributes::Attribute>& attributes);
	void on_connection_error(const boost::system::error_code& error);

public:
	Client(std::unique_ptr<Transport> transport, std::string host,
	       std::uint16_t port, RFCMode mode = RFCMode::RFC5389);
	~Client();

	void log_callback(LogCB callback, Verbosity verbosity);
	std::future<std::expected<attributes::MappedAddress, ErrorRet>> external_address();
	std::future<std::expected<std::vector<attributes::Attribute>, ErrorRet>> binding_request();
	std::future<std::expected<NAT, ErrorRet>> detect_nat_type();
	std::future<std::expected<bool, ErrorRet>> nat_present();
};

} // stun, ember