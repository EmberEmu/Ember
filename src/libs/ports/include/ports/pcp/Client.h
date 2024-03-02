/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/DatagramTransport.h>
#include <ports/pcp/Protocol.h>
#include <ports/pcp/Results.h>
#include <boost/asio/io_context.hpp>
#include <array>
#include <atomic>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <stack>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

namespace ember::ports {

using namespace std::literals;

constexpr int MAX_RETRIES = 9;
constexpr auto INITIAL_TIMEOUT = 250ms;

using Result = std::expected<MapRequest, Error>;
using RequestHandler = std::function<void(const Result& result)>;
using AnnounceHandler = std::function<void(std::uint32_t)>;

class Client {
private:
	enum class State {
		IDLE,
		AWAIT_MAP_RESULT_PCP,
		AWAIT_MAP_RESULT_NATPMP,
		AWAIT_EXTERNAL_ADDRESS_PCP,
		AWAIT_EXTERNAL_ADDRESS_NATPMP,
	};

	ba::io_context& ctx_;
	ba::steady_timer timer_;
	ba::io_context::strand strand_;
	DatagramTransport transport_;
	std::string gateway_;
	const std::string interface_;
	std::atomic_bool has_resolved_;
	std::atomic_bool resolve_res_;
	std::stack<State> states_;
	std::atomic<State> state_ { State::IDLE };
	std::stack<RequestHandler> handlers_;
	RequestHandler active_handler_;
	MapRequest stored_request_{};
	std::shared_ptr<std::vector<std::uint8_t>> buffer_;
	bool disable_natpmp_ = false;
	AnnounceHandler announce_handler_ = {};
	std::mutex handler_lock_;

	void start_retry_timer(std::chrono::milliseconds timeout = INITIAL_TIMEOUT, int retries = 0);
	void timeout_promise();

	void finagle_state();
	ErrorCode announce_pcp();
	void handle_connection_error();
	void send_request(std::vector<std::uint8_t> buffer);

	ErrorCode get_external_address_pmp();
	ErrorCode get_external_address_pcp();

	ErrorCode add_mapping_natpmp(const MapRequest& mapping);
	ErrorCode add_mapping_pcp(const MapRequest& mapping, bool strict);

	void handle_message(std::span<std::uint8_t> buffer, const boost::asio::ip::udp::endpoint& ep);
	Error parse_mapping_pcp(std::span<std::uint8_t> buffer, MapRequest& result);
	void handle_external_address_pcp(std::span<std::uint8_t> buffer);
	void handle_external_address_pmp(std::span<std::uint8_t> buffer);

	void handle_mapping_pcp(std::span<std::uint8_t> buffer);
	void handle_mapping_pmp(std::span<std::uint8_t> buffer);

	ErrorCode handle_pmp_to_pcp_error(std::span<std::uint8_t> buffer);
	bool handle_announce(std::span<std::uint8_t> buffer);

public:
	Client(const std::string& interface, std::string gateway, boost::asio::io_context& ctx);

	void external_address(RequestHandler handler);
	void add_mapping(const MapRequest& mapping, bool strict, RequestHandler handler);
	void delete_mapping(std::uint16_t internal_port, Protocol protocol, RequestHandler handler);
	void delete_all(Protocol protocol, RequestHandler handler);

	std::future<Result> external_address();
	std::future<Result> add_mapping(const MapRequest& mapping, bool strict = false);
	std::future<Result> delete_mapping(std::uint16_t internal_port, Protocol protocol);
	std::future<Result> delete_all(Protocol protocol);

	void announce_handler(AnnounceHandler&& handler);
	void disable_natpmp(bool disable);
};

} // ports, ember