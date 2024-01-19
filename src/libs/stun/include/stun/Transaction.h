/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Logging.h>
#include <stun/Protocol.h>
#include <boost/asio/steady_timer.hpp>
#include <array>
#include <chrono>
#include <expected>
#include <future>
#include <variant>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember::stun::detail {

constexpr std::chrono::milliseconds UDP_TX_TIMEOUT { 500 };
constexpr std::chrono::milliseconds TCP_TX_TIMEOUT { 39500 };
constexpr auto TX_RM { 16 }; // RFC drops magic number, refuses to elaborate
constexpr auto MAX_RETRIES = 7;

struct Transaction {
	Transaction(boost::asio::io_context& ctx,
		std::chrono::milliseconds timeout,
		int max_retries = MAX_RETRIES)
		: timer(ctx), timeout(timeout), initial_to(timeout),
		retries_left(max_retries), max_retries(max_retries) {}

	// :grimacing:
	using VariantPromise = std::variant<
		std::promise<std::expected<attributes::MappedAddress, Error>>,
		std::promise<std::expected<std::vector<attributes::Attribute>, Error>>
	>;

	TxID tx_id{};
	std::size_t hash{};
	std::chrono::milliseconds timeout;
	const std::chrono::milliseconds initial_to;
	VariantPromise promise;
	boost::asio::steady_timer timer;
	const int max_retries{};
	int retries_left{};
};

} // detail, stun, ember