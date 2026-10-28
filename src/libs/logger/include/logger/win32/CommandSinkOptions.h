/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>
#include <string>

namespace ember::log::win32 {

struct CommandSinkOptions {
	std::string name;
	std::string prompt        { "> "};
	Severity severity         { Severity::trace };
	Filter filter             { 0 };
	bool colours              { true };
	bool timestamp            { true };
	std::string timestamp_fmt { "%d/%m/%Y %H:%M:%S " };
	unsigned int prefix_width { 0 };
};

} // log, ember