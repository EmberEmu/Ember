/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/posix/Theme.h>

namespace ember::log::posix::themes {

inline constexpr ThemeView Daybreak {
	.name = "daybreak",
	.desc = "first light, it's time to go to bed",
	.tag {
		.trace  = "\033[38;2;162;172;168m",
		.debug  = "\033[38;2;157;187;224m",
		.info   = "\033[38;2;154;208;176m",
		.warn   = "\033[38;2;228;193;138m",
		.error  = "\033[38;2;229;160;154m",
		.fatal  = "\033[1;38;2;240;143;159m",
		.con    = "\033[38;2;190;174;221m",
		.conerr = "\033[38;2;229;160;154m",
	},
	.text {
		.trace  = "\033[38;2;162;172;168m",
		.debug  = "\033[38;2;157;187;224m",
		.info   = "\033[38;2;154;208;176m",
		.warn   = "\033[38;2;228;193;138m",
		.error  = "\033[38;2;229;160;154m",
		.fatal  = "\033[1;38;2;240;143;159m",
		.con    = "\033[38;2;190;174;221m",
		.conerr = "\033[38;2;229;160;154m",
	},
	.prompt     = "\033[38;2;154;208;176m",
	.input      = "\033[38;2;224;231;219m",
	.suggestion = "\033[38;2;162;172;168m",

	.bg         = "\033[48;2;48;59;56m",
	.timestamp  = "\033[38;2;133;150;140m",
	.reset      = "\033[39;22m"
};

} // themes, log, posix, ember