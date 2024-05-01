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
#include <stun/TransportBase.h>
#include <stun/Logging.h>
#include <boost/asio/io_context.hpp>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::stun {

const std::string_view SOFTWARE_DESC = "Ember";

using namespace detail;
using clientopts = int;
constexpr clientopts SUPPRESS_BANNER = 0x01;

enum class Protocol {
	TCP, UDP
};

class Client final {
	const int TX_RM = 16; // RFC drops magic number, refuses to elaborate
	const int MAX_REDIRECTS = 5;

	std::jthread worker_;
	std::unique_ptr<boost::asio::io_context::work> work_;
	boost::asio::io_context ctx_;

	Protocol proto_;
	std::unique_ptr<Transport> transport_;
	RFCMode mode_;
	LogCB logger_ = [](Verbosity, Error){};
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> dest_hist_;
	std::string host_;
	std::uint16_t port_;
	std::optional<bool> is_nat_present_;
	clientopts opts_{};
	std::unique_ptr<Transaction> tx_;
	std::mutex mutex_;

	// Transaction stuff
	void start_transaction_timer();
	void complete_transaction();
	void create_transaction(Transaction::Promise promise);
	void abort_transaction(Error error, attributes::ErrorCode = {}, bool erase = true);
	void rearm_transaction(State state, std::size_t key,
	                       std::shared_ptr<std::vector<std::uint8_t>> buffer,
	                       Transaction::TestData data = {});

	// Message handling stuff
	void handle_message(std::span<const std::uint8_t> buffer);
	void handle_binding_req();
	void handle_binding_resp();
	void handle_binding_err_resp();
	void handle_no_response();

	std::pair<std::shared_ptr<std::vector<std::uint8_t>>, std::size_t>
		build_request(bool change_ip = false, bool change_port = false);

	void process_message(std::span<const std::uint8_t> buffer);
	void connect(const std::string& host, std::uint16_t port, Transport::OnConnect&& cb);
	void set_nat_present();
	void on_connection_error(const boost::system::error_code& error);
	template<typename T> std::future<T> basic_request();
	template<typename T> std::future<T> behaviour_test(const State state);

	void perform_connectivity_test();
	void perform_mapping_test2();
	void perform_mapping_test3();
	void mapping_test_result();
	void perform_filtering_test2();
	void perform_filtering_test3();
	void perform_hairpinning_test();

public:
	Client(const std::string& bind, std::string host,
           std::uint16_t port, Protocol protocol,
	       RFCMode mode = RFCMode::RFC5780);
	~Client();

	void log_callback(LogCB callback);
	void options(clientopts opts);
	clientopts options() const;

	std::future<MappedResult> external_address();
	std::future<AttributesResult> binding_request();
	std::future<NATResult> nat_present();
	std::future<BehaviourResult> mapping();
	std::future<BehaviourResult> filtering();
	std::future<HairpinResult> hairpinning();
};

} // stun, ember