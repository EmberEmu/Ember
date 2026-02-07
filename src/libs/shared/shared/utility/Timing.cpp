/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Timing.h"
#include <boost/assert.hpp>
#include <gsl/narrow>

#ifdef _WIN32
	#include <Windows.h>
	#pragma comment(lib, "Winmm.lib")
#endif

namespace ember::util {

ScopedTimerPeriod::ScopedTimerPeriod(std::chrono::milliseconds ms)
	: success_(true),
	  restored_(false),
	  ms_(ms) {
	std::lock_guard guard(lock_);
	BOOST_ASSERT_MSG(valid(), "Bad consecutive ScopedTimerPeriod calls");
	++invokations_;
	set_timer();
}

// That's right, this only does something on Windows
// Not required for other platforms but it's nice to abstract it anyway
void ScopedTimerPeriod::set_timer() {
#ifdef _WIN32
	success_ = timeBeginPeriod(gsl::narrow<UINT>(ms_.count())) == TIMERR_NOERROR;
#endif
}

bool ScopedTimerPeriod::success() const {
	return success_;
}

void ScopedTimerPeriod::end() {
	std::lock_guard guard(lock_);
	
	if(!restored_) {
		timeEndPeriod(gsl::narrow<UINT>(ms_.count()));

		restored_ = true;
		--invokations_;
	}
}

ScopedTimerPeriod::~ScopedTimerPeriod() {
	end();
}

bool ScopedTimerPeriod::valid() {
	return !invokations_;
}

} // utils, ember