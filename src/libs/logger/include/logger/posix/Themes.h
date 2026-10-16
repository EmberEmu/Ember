/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/posix/LogTheme.h>
#include <array>
#include <span>
#include <stdexcept>

namespace ember::log::posix::themes {

inline constexpr LogTheme legacy {
	.name = "legacy",
	.desc = "original high-contrast logging theme, a touch of nostalgia",
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

	.bg         = "\033[48;00;00;00;36m",
	.timestamp  = "\033[01;30m",
	.reset      = "\033[39;22m"
};

inline constexpr LogTheme aurora {
	.name = "aurora",
	.desc = "aurora borealis but localised entirely within your terminal",
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

inline constexpr LogTheme aurora_day {
	.name = "aurora_day",
	.desc = "aurora borealis but it's daylight",
	.tag {
		.trace  = "\033[38;2;104;117;138m",
		.debug  = "\033[38;2;36;92;197m",
		.info   = "\033[38;2;8;127;112m",
		.warn   = "\033[38;2;152;96;0m",
		.error  = "\033[38;2;196;56;72m",
		.fatal  = "\033[1;38;2;194;24;66m",
		.con    = "\033[38;2;120;80;174m",
		.conerr = "\033[38;2;184;92;50m",
	},
	.text {
		.trace  = "\033[38;2;115;126;144m",
		.debug  = "\033[38;2;69;106;168m",
		.info   = "\033[38;2;50;127;117m",
		.warn   = "\033[38;2;144;110;53m",
		.error  = "\033[38;2;168;79;91m",
		.fatal  = "\033[1;38;2;181;45;75m",
		.con    = "\033[38;2;121;94;156m",
		.conerr = "\033[38;2;166;93;66m",
	},
	.prompt     = "\033[38;2;8;127;112m",
	.input      = "\033[38;2;38;52;69m",
	.suggestion = "\033[38;2;104;117;138m",

	.bg         = "\033[48;2;243;245;248m",
	.timestamp  = "\033[38;2;100;116;139m",
	.reset      = "\033[39;22m"
};

inline constexpr LogTheme smoky {
	.name = "smoky",
	.desc = "comfiest of themes, evoking wooden cabins and smoky, cosy fires",
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

inline constexpr LogTheme abyssal {
	.name = "abyssal",
	.desc = "in the depths of the ocean, mysterious log messages dwell",
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

inline constexpr LogTheme sahara {
	.name = "sahara",
	.desc = "sand spills across your terminal, shimmering dunes during a glorious sunset",
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

inline constexpr LogTheme regalia {
	.name = "regalia",
	.desc = "fit for a king, enchanting your log messages with +5 opulence",
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

inline constexpr LogTheme midnight {
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

inline constexpr std::array themes {
	legacy,
	aurora,
	aurora_day,
	abyssal,
	smoky,
	sahara,
	regalia,
	midnight
};

constexpr const LogTheme& theme(std::string_view name) {
	for(const auto& theme : themes) {
		if(theme.name == name) {
			return theme;
		}
	}

	throw std::runtime_error("unknown theme requested");
}

constexpr const std::span<const LogTheme> get_themes() {
	return themes;
}

} // themes, posix, log, ember