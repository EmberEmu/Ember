/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <atomic>

namespace ember::realm {

class ClientSlots {
	inline static std::atomic_size_t curr_clients_;
	inline static std::atomic_size_t peak_clients_;

	static void update_peak();

public:
	static std::size_t curr_clients();
	static std::size_t peak_clients();
	static bool reserve_slot(std::size_t limit);
	static void free_slot();

	friend class Client;
};

} // realm, ember