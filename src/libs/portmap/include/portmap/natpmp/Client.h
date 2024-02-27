/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <portmap/natpmp/DatagramTransport.h>
#include <portmap/natpmp/Protocol.h>
#include <portmap/natpmp/Results.h>
#include <array>
#include <atomic>
#include <expected>
#include <algorithm>
#include <future>
#include <stack>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

namespace ember::portmap::natpmp {

class Client {
	enum class State {
		IDLE,
		AWAITING_MAPPING_RESULT_PMP,
		AWAITING_MAPPING_RESULT_PCP,
		AWAITING_DELETE_RESULT_PMP,
		AWAITING_DELETE_RESULT_PCP,
		AWAITING_EXTERNAL_ADDRESS_PMP,
		AWAITING_EXTERNAL_ADDRESS_PCP,
	};

	using MapResult = std::expected<MappingResult, Error>;
	using ExternalAddress = std::expected<std::array<std::uint8_t, 16>, Error>;

	using ClientPromise = std::variant<
		std::promise<MapResult>, std::promise<ExternalAddress>
	>;

	DatagramTransport transport_;
	const std::string gateway_;
	const std::string interface_;
	std::atomic_bool has_resolved_;
	std::atomic_bool resolve_res_;
	std::stack<State> states_;
	std::atomic<State> state_ { State::IDLE };
	std::stack<ClientPromise> promises_;
	ClientPromise prev_promise_;
	ClientPromise active_promise_;

	void finagle_state();
	void announce_pcp();
	void handle_connection_error();

	void get_external_address_pmp(std::promise<ExternalAddress> promise);
	void get_external_address_pcp(std::promise<ExternalAddress> promise);

	void add_mapping_natpmp(const RequestMapping& mapping, std::promise<MapResult> promise);
	void add_mapping_pcp(const RequestMapping& mapping, std::promise<MapResult> promise);
	void do_delete_mapping(std::uint16_t internal_port, Protocol protocol,
	                       std::promise<MapResult> promise);

	void handle_message(std::span<std::uint8_t> buffer, const boost::asio::ip::udp::endpoint& ep);
	void handle_external_address(std::span<std::uint8_t> buffer);
	void handle_external_address_pcp(std::span<std::uint8_t> buffer);
	void handle_external_address_pmp(std::span<std::uint8_t> buffer);

	void handle_mapping_pcp(std::span<std::uint8_t> buffer);
	void handle_mapping_pmp(std::span<std::uint8_t> buffer);

	ErrorType handle_pmp_to_pcp_error(std::span<std::uint8_t> buffer);

public:
	Client(const std::string& interface, std::string gateway);

	std::future<ExternalAddress> external_address();
	std::future<MapResult> add_mapping(RequestMapping mapping);
	std::future<MapResult> delete_mapping(std::uint16_t internal_port, Protocol protocol);
	std::future<MapResult> delete_all(Protocol protocol);
};

} // natpmp, portmap, ember