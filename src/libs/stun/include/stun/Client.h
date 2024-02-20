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
#include <stun/Transport.h>
#include <stun/Util.h>
#include <boost/asio/io_context.hpp>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::spark {
	class BinaryInStream;
}

namespace ember::stun {

const std::string_view SOFTWARE_DESC = "Ember";

using clientopts = int;
constexpr clientopts SUPPRESS_BANNER = 0x01;

class Client {
	const int TX_RM = 16; // RFC drops magic number, refuses to elaborate
	const int MAX_REDIRECTS = 5;

	std::jthread worker_;
	std::vector<std::shared_ptr<boost::asio::io_context::work>> work_;

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
	clientopts opts_{};

	/*
	 * The transaction map is protected by locking at each 'entry point' into
	 * the class. The user entry points are the public functions and the
	 * transport worker entry points are defined by the callbacks.
	 */
	std::unordered_map<std::size_t, detail::Transaction> transactions_;
	std::mutex mutex_;

	// Transaction stuff
	detail::Transaction& start_transaction(std::shared_ptr<detail::Transaction::Promise> promise,
	                                       std::shared_ptr<std::vector<std::uint8_t>> data,
	                                       std::size_t key);

	void start_transaction_timer(detail::Transaction& tx);

	void complete_transaction(detail::Transaction& tx,
	                          std::vector<attributes::Attribute> attributes);

	void abort_transaction(detail::Transaction& tx, Error error,
	                       attributes::ErrorCode = {}, bool erase = true);

	void abort_promise(std::shared_ptr<detail::Transaction::Promise> promise, Error error);

	// Message handling stuff
	void handle_message(std::vector<std::uint8_t> buffer);
	void handle_binding_resp(std::vector<attributes::Attribute> attributes,
	                         detail::Transaction& tx);
	void handle_binding_err_resp(const std::vector<attributes::Attribute>& attributes,
	                             detail::Transaction& tx);
	void binding_request(std::shared_ptr<detail::Transaction::Promise> promise);
	void process_message(const std::vector<std::uint8_t>& buffer, detail::Transaction& tx);
	void connect(const std::string& host, std::uint16_t port, Transport::OnConnect cb);
	void set_nat_present(const std::vector<attributes::Attribute>& attributes);
	void on_connection_error(const boost::system::error_code& error);
	template<typename T> std::future<T> basic_request();

public:
	Client(std::unique_ptr<Transport> transport, std::string host,
	       std::uint16_t port, RFCMode mode = RFCMode::RFC5780);
	~Client();

	void log_callback(LogCB callback, Verbosity verbosity);
	void options(clientopts opts);
	clientopts options() const;

	std::future<std::expected<attributes::MappedAddress, ErrorRet>> external_address();
	std::future<std::expected<std::vector<attributes::Attribute>, ErrorRet>> binding_request();
	std::future<std::expected<NAT, ErrorRet>> nat_type();
	std::future<std::expected<bool, ErrorRet>> nat_present();
	std::future<std::expected<Mapping, ErrorRet>> mapping();
	std::future<std::expected<Filtering, ErrorRet>> filtering();
};

} // stun, ember