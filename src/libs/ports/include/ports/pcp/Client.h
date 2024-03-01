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
#include <stack>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

namespace ember::ports {

using namespace std::literals;

constexpr int MAX_RETRIES = 9;
constexpr auto INITIAL_TIMEOUT = 250ms;

class Client {
public:
	using MapResult = std::expected<MappingResult, Error>;
	using ExternalAddress = std::expected<std::array<std::uint8_t, 16>, Error>;

	using ClientPromise = std::variant<
		std::promise<MapResult>, std::promise<ExternalAddress>
	>;

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
	std::stack<ClientPromise> promises_;
	ClientPromise active_promise_;
	MapRequest stored_request_{};
	std::shared_ptr<std::vector<std::uint8_t>> buffer_;
	bool disable_natpmp_ = false;

	void start_retry_timer(std::chrono::milliseconds timeout = INITIAL_TIMEOUT, int retries = 0);
	void timeout_promise();

	void finagle_state();
	ErrorType announce_pcp();
	void handle_connection_error();
	void send_request(std::vector<std::uint8_t> buffer);

	ErrorType get_external_address_pmp();
	ErrorType get_external_address_pcp();

	ErrorType add_mapping_natpmp(const MapRequest& mapping);
	ErrorType add_mapping_pcp(const MapRequest& mapping);

	void handle_message(std::span<std::uint8_t> buffer, const boost::asio::ip::udp::endpoint& ep);
	Error parse_mapping_pcp(std::span<std::uint8_t> buffer, MappingResult& result);
	void handle_external_address_pcp(std::span<std::uint8_t> buffer);
	void handle_external_address_pmp(std::span<std::uint8_t> buffer);

	void handle_mapping_pcp(std::span<std::uint8_t> buffer);
	void handle_mapping_pmp(std::span<std::uint8_t> buffer);

	ErrorType handle_pmp_to_pcp_error(std::span<std::uint8_t> buffer);

public:
	Client(const std::string& interface, std::string gateway, boost::asio::io_context& ctx);

	std::future<ExternalAddress> external_address();
	std::future<MapResult> add_mapping(const MapRequest& mapping);
	std::future<MapResult> delete_mapping(std::uint16_t internal_port, Protocol protocol);
	std::future<MapResult> delete_all(natpmp::Protocol protocol);
	void disable_natpmp(bool disable);
};

} // ports, ember