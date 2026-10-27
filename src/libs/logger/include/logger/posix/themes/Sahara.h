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

inline constexpr ThemeView Sahara {
	.name = "sahara",
<<<<<<< HEAD
	.desc = "Sand spills across your terminal, shimmering dunes reflect a sanguine sunset",
=======
	.desc = "sand spills across your terminal, shimmering dunes during a glorious sunset",
	.prefix {
		.trace  = "\033[38;2;119;114;143m",
		.debug  = "\033[38;2;119;114;143m",
		.info   = "\033[38;2;119;114;143m",
		.warn   = "\033[38;2;119;114;143m",
		.error  = "\033[38;2;119;114;143m",
		.fatal  = "\033[38;2;119;114;143m",
		.con    = "\033[38;2;119;114;143m",
		.conerr = "\033[38;2;119;114;143m",
	},
>>>>>>> fdd0e2e4 (Add theming section for prefixes)
	.tag {
		.trace  = "\033[38;2;165;161;191m",
		.debug  = "\033[38;2;165;182;232m",
		.info   = "\033[38;2;217;182;111m",
		.warn   = "\033[38;2;240;184;92m",
		.error  = "\033[38;2;232;141;114m",
		.fatal  = "\033[1;38;2;255;107;98m",
		.con    = "\033[38;2;196;160;217m",
		.conerr = "\033[38;2;232;141;114m",
	},
	.text {
		.trace  = "\033[38;2;208;195;174m",
		.debug  = "\033[38;2;210;214;234m",
		.info   = "\033[38;2;240;217;168m",
		.warn   = "\033[38;2;232;201;160m",
		.error  = "\033[38;2;235;192;168m",
		.fatal  = "\033[1;38;2;255;176;160m",
		.con    = "\033[38;2;224;203;232m",
		.conerr = "\033[38;2;235;192;168m",
	},
	.prompt     = "\033[38;2;217;182;111m",
	.input      = "\033[38;2;240;217;168m",
	.suggestion = "\033[38;2;165;161;191m",

	.bg         = "\033[48;2;23;19;33m",
	.timestamp  = "\033[38;2;119;114;143m",
	.reset      = "\033[39;22m"
};

} // themes, log, posix, ember