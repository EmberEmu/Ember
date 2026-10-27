/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <cstddef>

namespace ember {

struct RateLimitOptions final {
	std::size_t start_tokens;
	std::size_t max_tokens;
	std::chrono::nanoseconds refill_rate;
	std::size_t refill_count;
};

class RateLimit final {
	const RateLimitOptions options_;
	mutable std::size_t tokens_;
	mutable std::chrono::nanoseconds previous_tick_;

	void refill() const noexcept;

public:
	RateLimit(RateLimitOptions options);

	bool try_consume(std::size_t cost = 1) noexcept;
	std::size_t add_tokens(std::size_t tokens) noexcept;
	std::size_t remaining() const noexcept;
	std::size_t capacity() const noexcept;
	void reset() noexcept;
	std::chrono::nanoseconds time_until_available(std::size_t cost) const noexcept;
};

} // ember