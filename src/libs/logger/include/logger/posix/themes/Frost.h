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

inline constexpr ThemeView Frost {
    .name = "frost",
    .desc = "A theme fit for a fictional prince. A very cold, copyrighted prince.",
	.prefix {
        .trace  = "\033[38;2;82;107;143m",
        .debug  = "\033[38;2;82;107;143m",
        .info   = "\033[38;2;82;107;143m",
        .warn   = "\033[38;2;82;107;143m",
        .error  = "\033[38;2;82;107;143m",
        .fatal  = "\033[38;2;82;107;143m",
        .con    = "\033[38;2;82;107;143m",
        .conerr = "\033[38;2;82;107;143m",
    },
    .tag {
        .trace  = "\033[38;2;138;171;204m",
        .debug  = "\033[38;2;61;155;255m",
        .info   = "\033[38;2;0;245;255m",
        .warn   = "\033[38;2;255;208;0m",
        .error  = "\033[38;2;255;56;100m",
        .fatal  = "\033[1;38;2;255;23;68m",
        .con    = "\033[38;2;181;107;255m",
        .conerr = "\033[38;2;255;107;53m",
    },
    .text {
        .trace  = "\033[38;2;181;203;229m",
        .debug  = "\033[38;2;156;203;255m",
        .info   = "\033[38;2;165;255;255m",
        .warn   = "\033[38;2;255;240;160m",
        .error  = "\033[38;2;255;154;175m",
        .fatal  = "\033[1;38;2;255;82;119m",
        .con    = "\033[38;2;220;192;255m",
        .conerr = "\033[38;2;255;177;143m",
    },
    .prompt     = "\033[1;38;2;0;245;255m",
    .input      = "\033[38;2;235;249;255m",
    .suggestion = "\033[38;2;82;107;143m",

    .bg         = "\033[48;2;8;15;32m",
    .timestamp  = "\033[38;2;82;107;143m",
    .reset      = "\033[39;22m"
};

} // themes, log, posix, ember