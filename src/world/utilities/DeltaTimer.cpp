/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DeltaTimer.h"

using namespace std::chrono_literals;

namespace ember {

DeltaTimer::DeltaTimer(std::chrono::milliseconds interval)
	: interval_(std::move(interval)),
	  elapsed_(0) {}

bool DeltaTimer::elapsed() const {
	return elapsed_ >= interval_;
}

bool DeltaTimer::update(const std::chrono::milliseconds& delta) {
	elapsed_ += delta;
	return elapsed();
}

bool DeltaTimer::consume() {
	if(elapsed()) {
		elapsed_ -= interval_;
		return true;
	} else {
		return false;
	}
}

void DeltaTimer::reset() {
	elapsed_ = 0ms;
}

const std::chrono::milliseconds& DeltaTimer::value() const {
	return elapsed_;
}

const std::chrono::milliseconds& DeltaTimer::interval() const {
	return interval_;
}

} // ember