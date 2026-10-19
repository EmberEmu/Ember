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

inline constexpr ThemeView Midnight {
	.name = "midnight",
	.desc = "easy on weary eyes, staring at debug logs as the night slips away, once again",
	.tag {
		.trace  = "\033[1;38;2;105;117;145m",
		.debug  = "\033[1;38;2;137;180;250m",
		.info   = "\033[1;38;2;139;213;202m",
		.warn   = "\033[1;38;2;245;194;125m",
		.error  = "\033[1;38;2;243;139;168m",
		.fatal  = "\033[1;97;48;2;121;29;53m",
		.con    = "\033[1;38;2;203;166;247m",
		.conerr = "\033[1;38;2;250;179;135m",
	},
	.text {
		.trace  = "\033[38;2;137;148;176m",
		.debug  = "\033[38;2;186;199;225m",
		.info   = "\033[38;2;184;218;215m",
		.warn   = "\033[38;2;230;208;169m",
		.error  = "\033[38;2;235;184;198m",
		.fatal  = "\033[1;38;2;255;160;177m",
		.con    = "\033[38;2;215;198;238m",
		.conerr = "\033[38;2;235;195;173m",
	},
	.prompt     = "\033[38;2;137;213;202m",
	.input      = "\033[38;2;220;226;241m",
	.suggestion = "\033[38;2;105;117;145m",

	.bg        = "\033[48;2;3;5;9m",
	.timestamp = "\033[38;2;53;64;87m",
	.reset     = "\033[39;22m"
};

} // themes, log, posix, ember