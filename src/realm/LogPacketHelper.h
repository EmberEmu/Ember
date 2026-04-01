/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "FilterTypes.h"
#include <logger/HelperMacros.h>

#define PACKET_LOG(logger, fmt_str, ...) \
LOG_FTRACE(logger, log::Filter(lf_packet_log), fmt_str  __VA_OPT__(,) __VA_ARGS__)