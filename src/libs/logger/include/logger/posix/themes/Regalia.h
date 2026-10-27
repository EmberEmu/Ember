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

inline constexpr ThemeView Regalia {
	.name = "regalia",
	.desc = "Fit for a king, enchanting your log messages with +5 opulence",
	.prefix {
		.trace  = "\033[38;2;129;116;156m",
		.debug  = "\033[38;2;129;116;156m",
		.info   = "\033[38;2;129;116;156m",
		.warn   = "\033[38;2;129;116;156m",
		.error  = "\033[38;2;129;116;156m",
		.fatal  = "\033[38;2;129;116;156m",
		.con    = "\033[38;2;129;116;156m",
		.conerr = "\033[38;2;129;116;156m",
	},
	.tag {
		.trace  = "\033[38;2;182;162;217m",
		.debug  = "\033[38;2;96;165;250m",
		.info   = "\033[1;38;2;255;215;0m",
		.warn   = "\033[38;2;251;191;36m",
		.error  = "\033[38;2;251;113;133m",
		.fatal  = "\033[1;38;2;255;245;245;48;2;160;0;35m",
		.con    = "\033[38;2;192;132;252m",
		.conerr = "\033[38;2;52;211;153m",
	},
	.text {
		.trace  = "\033[38;2;212;198;239m",
		.debug  = "\033[38;2;185;216;255m",
		.info   = "\033[38;2;255;241;174m",
		.warn   = "\033[38;2;255;224;160m",
		.error  = "\033[38;2;255;192;203m",
		.fatal  = "\033[1;38;2;255;158;173m",
		.con    = "\033[38;2;233;213;255m",
		.conerr = "\033[38;2;167;243;208m",
	},
	.prompt     = "\033[38;2;255;215;0m",
	.input      = "\033[38;2;250;245;255m",
	.suggestion = "\033[38;2;182;162;217m",

	.bg         = "\033[48;2;16;9;31m",
	.timestamp  = "\033[38;2;129;116;156m",
	.reset      = "\033[39;22m"
};

} // themes, log, posix, ember