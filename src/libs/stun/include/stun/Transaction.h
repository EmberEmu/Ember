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
#include <memory>
#include <variant>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember::stun::detail {

struct Transaction {
	Transaction(boost::asio::any_io_executor ctx, std::chrono::milliseconds timeout, int max_retries)
		: timer(ctx), timeout(timeout), initial_to(timeout),
		retries_left(max_retries), max_retries(max_retries) {}

	// :grimacing:
	using Promise = std::variant<
		std::promise<std::expected<attributes::MappedAddress, ErrorRet>>,
		std::promise<std::expected<std::vector<attributes::Attribute>, ErrorRet>>,
		std::promise<std::expected<NAT, ErrorRet>>,
		std::promise<std::expected<bool, ErrorRet>>,
		std::promise<std::expected<Filtering, ErrorRet>>,
		std::promise<std::expected<Mapping, ErrorRet>>
	>;

	TxID id{};
	std::size_t key{};
	std::chrono::milliseconds timeout;
	const std::chrono::milliseconds initial_to;
	std::shared_ptr<Promise> promise;
	boost::asio::steady_timer timer;
	const int max_retries{};
	int retries_left{};
	int redirects{};
	std::shared_ptr<std::vector<std::uint8_t>> retry_buffer;
};

} // detail, stun, ember