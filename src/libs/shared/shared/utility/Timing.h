/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <utility>

namespace ember::util {

/*
 * Allows for setting the timer period, ensuring that
 * it's reset to its original value when leaving scope
 */
class ScopedTimerPeriod final {
	using Callback = std::function<void()>;

	bool success_;
	bool restored_;
	const std::chrono::milliseconds ms_;
	
	static std::mutex lock_;
	static int invokations_;

	void set_timer();

public:
	ScopedTimerPeriod(std::chrono::milliseconds ms);
	~ScopedTimerPeriod();

	bool success() const;
	void end();

	static bool valid();
};

} // util, ember