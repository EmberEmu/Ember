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

inline constexpr ThemeView Smoky {
	.name = "smoky",
	.desc = "Comfy theme, evoking wooden cabins and smoky fires",
	.tag {
		.trace  = "\033[38;2;163;148;133m",
		.debug  = "\033[38;2;209;154;106m",
		.info   = "\033[38;2;230;181;102m",
		.warn   = "\033[38;2;240;163;74m",
		.error  = "\033[38;2;240;120;69m",
		.fatal  = "\033[1;38;2;255;92;53m",
		.con    = "\033[38;2;214;160;200m",
		.conerr = "\033[38;2;233;138;101m",
	},
	.text {
		.trace  = "\033[38;2;176;161;148m",
		.debug  = "\033[38;2;196;161;132m",
		.info   = "\033[38;2;214;185;141m",
		.warn   = "\033[38;2;217;173;127m",
		.error  = "\033[38;2;221;160;138m",
		.fatal  = "\033[1;38;2;255;152;120m",
		.con    = "\033[38;2;212;176;202m",
		.conerr = "\033[38;2;221;176;154m",
	},
	.prompt     = "\033[38;2;230;181;102m",
	.input      = "\033[38;2;241;230;217m",
	.suggestion = "\033[38;2;163;148;133m",

	.bg         = "\033[48;2;33;28;25m",
	.timestamp  = "\033[38;2;136;121;107m",
	.reset      = "\033[39;22m"
};

} // themes, log, posix, ember