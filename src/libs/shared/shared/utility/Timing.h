/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <mutex>
#include <utility>

namespace ember::utility {

/*
 * Allows for setting the timer period, ensuring that
 * it's reset to its original value when leaving scope
 */
class ScopedTimerPeriod final {
	bool success_;
	bool restored_;
	const std::chrono::milliseconds ms_;
	
	inline static std::mutex lock_;
	inline static int invokations_;

	void set_timer();

public:
	explicit ScopedTimerPeriod(std::chrono::milliseconds ms);
	~ScopedTimerPeriod();

	bool success() const;
	void end();

	static bool valid();
};

} // utility, ember