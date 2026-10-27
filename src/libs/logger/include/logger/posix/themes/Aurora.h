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

inline constexpr ThemeView Aurora {
	.name = "aurora",
	.desc = "Aurora borealis, localised entirely within your terminal",
	.prefix {
		.trace  = "\033[38;2;100;113;138m",
		.debug  = "\033[38;2;100;113;138m",
		.info   = "\033[38;2;100;113;138m",
		.warn   = "\033[38;2;100;113;138m",
		.error  = "\033[38;2;100;113;138m",
		.fatal  = "\033[38;2;100;113;138m",
		.con    = "\033[38;2;100;113;138m",
		.conerr = "\033[38;2;100;113;138m",
	},
	.tag {
		.trace  = "\033[38;2;132;145;168m",
		.debug  = "\033[38;2;130;170;255m",
		.info   = "\033[38;2;93;228;199m",
		.warn   = "\033[38;2;255;213;128m",
		.error  = "\033[38;2;255;128;128m",
		.fatal  = "\033[1;38;2;255;83;112m",
		.con    = "\033[38;2;196;167;231m",
		.conerr = "\033[38;2;255;173;143m",
	},
	.text {
		.trace  = "\033[38;2;165;174;192m",
		.debug  = "\033[38;2;178;201;242m",
		.info   = "\033[38;2;162;233;217m",
		.warn   = "\033[38;2;233;213;168m",
		.error  = "\033[38;2;240;176;176m",
		.fatal  = "\033[1;38;2;255;155;170m",
		.con    = "\033[38;2;213;197;235m",
		.conerr = "\033[38;2;240;197;180m",
	},
	.prompt     = "\033[38;2;93;228;199m",
	.input      = "\033[38;2;232;236;248m",
	.suggestion = "\033[38;2;132;145;168m",

	.bg         = "\033[48;2;23;27;43m",
	.timestamp  = "\033[38;2;100;113;138m",
	.reset      = "\033[39;22m"
};

} // themes, log, posix, ember