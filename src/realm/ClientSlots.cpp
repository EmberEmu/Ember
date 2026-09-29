/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientSlots.h"

namespace ember::realm {

std::size_t ClientSlots::curr_clients() {
	return curr_clients_;
}

std::size_t ClientSlots::peak_clients() {
	return peak_clients_;
}

void ClientSlots::free_slot() {
	--curr_clients_;
}

void ClientSlots::update_peak() {
	// relaxed load is friendlier to cache than immediately trying to exchange
	auto current = curr_clients_.load(std::memory_order_relaxed);
	auto peak = peak_clients_.load(std::memory_order_relaxed);

	while(current > peak) {
		if(peak_clients_.compare_exchange_weak(peak, current, std::memory_order_relaxed)) {
			return;
		}
	}
}

bool ClientSlots::reserve_slot(const std::size_t limit) {
	const auto old_count = curr_clients_.fetch_add(1, std::memory_order_acquire);

	// roll the counter back
	if(old_count >= limit) {
		curr_clients_.fetch_sub(1, std::memory_order_release);
		return false;
	}

	return true;
}

} // realm, ember