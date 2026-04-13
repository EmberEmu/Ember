/*
* Copyright (c) 2020 - 2026 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "FilterTypes.h"
#include <logger/HelperMacros.h>

#define PACKET_TRACE(ctx, fmt_str, ...) \
	LOG_FTRACE(ctx.logger, log::Filter(lf_packet_trace), "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_TRACE(ctx, fmt_str, ...) \
	LOG_TRACE(ctx.logger, "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_DEBUG(ctx, fmt_str, ...) \
	LOG_DEBUG(ctx.logger, "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_INFO(ctx, fmt_str, ...) \
	LOG_INFO(ctx.logger, "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_WARN(ctx, fmt_str, ...) \
    LOG_WARN(ctx.logger, "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_ERROR(ctx, fmt_str, ...) \
	LOG_ERROR(ctx.logger, "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_FATAL(ctx, fmt_str, ...) \
	LOG_FATAL(ctx.logger, "{}: " fmt_str, ctx.client_identify(), ##__VA_ARGS__)

#define CLIENT_TRACE_GLOB(ctx, fmt_str, ...) \
	CLIENT_TRACE(ember::log::global_ctx.logger(), ctx, ##__VA_ARGS__)

#define CLIENT_DEBUG_GLOB(ctx, fmt_str, ...) \
	CLIENT_DEBUG(ember::log::global_ctx.logger(), ctx, ##__VA_ARGS__)

#define CLIENT_INFO_GLOB(ctx, fmt_str, ...) \
	CLIENT_INFO(ember::log::global_ctx.logger(), ctx, ##__VA_ARGS__)

#define CLIENT_WARN_GLOB(ctx, fmt_str, ...) \
	CLIENT_WARN(ember::log::global_ctx.logger(), ctx, ##__VA_ARGS__)

#define CLIENT_ERROR_GLOB(ctx, fmt_str, ...) \
	CLIENT_ERROR(ember::log::global_ctx.logger(), ctx, ##__VA_ARGS__)

#define CLIENT_FATAL_GLOB(ctx, fmt_str, ...) \
	CLIENT_FATAL(ember::log::global_ctx.logger(), ctx, ##__VA_ARGS__)
