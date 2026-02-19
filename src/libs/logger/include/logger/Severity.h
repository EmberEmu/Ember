/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/serialization/strong_typedef.hpp>
#include <cstdint>

namespace ember::log {

enum class Severity {
	trace,
	debug,
	info,
	warn,
	ERROR_,
	fatal,
	console,
	console_error,
	disabled,
	severity_max = disabled
};

BOOST_STRONG_TYPEDEF(std::uint32_t, Filter);

struct RecordDetail {
	Severity severity;
	Filter type;
};

} // log, ember