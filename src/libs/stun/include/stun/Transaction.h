/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Logging.h>
#include <array>
#include <chrono>
#include <expected>
#include <future>
#include <variant>
#include <cstddef>
#include <cstdint>

namespace ember::stun::detail {

struct Transaction {
	// :grimacing:
	using VariantPromise = std::variant <
		std::promise<std::expected<attributes::MappedAddress, Error>>,
		std::promise<std::expected<std::vector<attributes::Attribute>, Error>>
	>;

	std::array<std::uint32_t, 4> tx_id;
	std::size_t hash;
	std::uint8_t retries;
	std::chrono::milliseconds retry_timeout;
	VariantPromise promise;
};

} // detail, stun, ember