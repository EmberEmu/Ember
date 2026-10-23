/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ratelimit/RateLimit.h>
#include <shared/utility/TickCount.h>
#include <algorithm>
#include <cassert>

using namespace std::chrono_literals;

namespace ember {

RateLimit::RateLimit(RateLimitOptions options)
	: options_(std::move(options))
	, tokens_(options_.start_tokens)
	, previous_tick_(utility::get_tick_count(utility::as_chrono)) {
	assert(options_.refill_rate > 0ns);
}

bool RateLimit::try_consume(const std::size_t cost) noexcept {
	refill();

	if(tokens_ >= cost) {
		tokens_ -= cost;
		return true;
	}
	
	return false;
}

void RateLimit::refill() const noexcept {
	const auto current_tick = utility::get_tick_count(utility::as_chrono);
	const auto elapsed = current_tick - previous_tick_;
	const auto intervals = elapsed / options_.refill_rate;

	if(intervals <= 0) {
		return;
	}

	const auto capacity = options_.max_tokens - tokens_;
	const auto max_intervals = capacity / options_.refill_count;

	const auto added = (intervals >= max_intervals)?
		capacity : static_cast<std::size_t>(intervals) * options_.refill_count;

	tokens_ += added;
	previous_tick_ += options_.refill_rate * intervals;
}

std::size_t RateLimit::remaining() const noexcept {
	return tokens_;
}

std::size_t RateLimit::add_tokens(const std::size_t tokens) noexcept {
	const auto capacity = options_.max_tokens - tokens_;
	const auto add = std::min(capacity, tokens);
	tokens_ += add;
	return add;
}

void RateLimit::reset() noexcept {
	tokens_ = options_.start_tokens;
	previous_tick_ = utility::get_tick_count(utility::as_chrono);
}

std::size_t RateLimit::capacity() const noexcept {
	return options_.max_tokens;
}

std::chrono::nanoseconds RateLimit::time_until_available(const std::size_t cost) const noexcept {
	refill();

	if (cost <= tokens_) {
		return std::chrono::nanoseconds::zero();
	}

	if (cost > options_.max_tokens) {
		return std::chrono::nanoseconds::max();
	}

	const auto needed = cost - tokens_;
	const auto intervals = (needed + options_.refill_count - 1) / options_.refill_count;
	const auto elapsed = utility::get_tick_count(utility::as_chrono) - previous_tick_;
	const auto remainder = elapsed % options_.refill_rate;

	return options_.refill_rate * intervals - remainder;
}

} // ember