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

inline constexpr ThemeView Abyssal {
	.name = "abyssal",
	.desc = "In the depths of the terminal, mysterious log messages dwell",
	.prefix {
		.trace  =  "\033[38;2;96;119;130m",
		.debug  =  "\033[38;2;96;119;130m",
		.info   =  "\033[38;2;96;119;130m",
		.warn   =  "\033[38;2;96;119;130m",
		.error  =  "\033[38;2;96;119;130m",
		.fatal  =  "\033[38;2;96;119;130m",
		.con    =  "\033[38;2;96;119;130m",
		.conerr =  "\033[38;2;96;119;130m",
	},
	.tag {
		.trace  = "\033[38;2;113;137;149m",
		.debug  = "\033[38;2;124;184;217m",
		.info   = "\033[38;2;99;230;190m",
		.warn   = "\033[38;2;242;192;120m",
		.error  = "\033[38;2;255;123;114m",
		.fatal  = "\033[1;38;2;255;84;112m",
		.con    = "\033[38;2;196;167;231m",
		.conerr = "\033[38;2;255;160;122m",
	},
	.text {
		.trace  = "\033[38;2;145;162;170m",
		.debug  = "\033[38;2;167;205;224m",
		.info   = "\033[38;2;160;235;210m",
		.warn   = "\033[38;2;228;206;167m",
		.error  = "\033[38;2;240;170;164m",
		.fatal  = "\033[1;38;2;255;154;167m",
		.con    = "\033[38;2;216;200;237m",
		.conerr = "\033[38;2;240;187;167m",
	},
	.prompt     = "\033[38;2;99;230;190m",
	.input      = "\033[38;2;228;241;243m",
	.suggestion = "\033[38;2;97;121;131m",

	.bg        = "\033[48;2;11;20;24m",
	.timestamp = "\033[38;2;96;119;130m",
	.reset     = "\033[39;22m"
};

} // themes, log, posix, ember