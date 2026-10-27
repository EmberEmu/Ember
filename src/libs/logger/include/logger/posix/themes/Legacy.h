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

inline constexpr ThemeView Legacy {
	.name = "legacy",
	.desc = "Original high-contrast logging theme for a touch of nostalgia",
	.prefix {
		.trace  = "\033[01;30m",
		.debug  = "\033[01;30m",
		.info   = "\033[01;30m",
		.warn   = "\033[01;30m",
		.error  = "\033[01;30m",
		.fatal  = "\033[01;30m",
		.con    = "\033[01;30m",
		.conerr = "\033[01;30m",
	},
	.tag {
		.trace  = "\033[01;30m",
		.debug  = "\033[01;36m",
		.info   = "\033[01;37m",
		.warn   = "\033[01;31m",
		.error  = "\033[01;31m",
		.fatal  = "\033[01;31m",
		.con    = "\033[01;37m\033[46m",
		.conerr = "\033[01;37m\033[41m",
	},
	.text {
		.trace  = "\033[01;30m",
		.debug  = "\033[01;36m",
		.info   = "\033[01;37m",
		.warn   = "\033[01;31m",
		.error  = "\033[01;31m",
		.fatal  = "\033[01;31m",
		.con    = "\033[01;37m\033[46m",
		.conerr = "\033[01;37m\033[41m",
	},
	.prompt     = "\033[01;30m",
	.input      = "\033[01;37m",
	.suggestion = "\033[01;30m",

	.bg         = "\033[48;2;0;0;0m",
	.timestamp  = "\033[01;30m",
	.reset      = "\033[39;22m"
};

} // themes, log, posix, ember