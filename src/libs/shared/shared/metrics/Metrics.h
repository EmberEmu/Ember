/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <string>
#include <cstdint>

namespace ember {

class Metrics {
public:
	enum class Adjustment { NONE, POSITIVE, NEGATIVE };

	virtual void increment(const char* key, std::intmax_t value = 1) { }
	virtual void timing(const char* key, std::chrono::milliseconds value) { }
	virtual void gauge(const char* key, std::uintmax_t value, Adjustment adjustment = Adjustment::NONE) { }
	virtual void set(const char* key, std::intmax_t value) { }
	virtual ~Metrics() = default;
};

} // ember