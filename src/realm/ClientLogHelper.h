/*
* Copyright (c) 2020 - 2026 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "FilterTypes.h"
#include <logger/Logger.h>

#define PACKET_TRACE(logger, ctx) \
	LOG_TRACE_STREAM(logger) << log::Filter(lf_packet_trace) << ctx.handler.client_identify()

#define CLIENT_TRACE(logger, ctx) \
	LOG_TRACE_STREAM(logger) << ctx.handler.client_identify() << ": "

#define CLIENT_DEBUG(logger, ctx) \
	LOG_DEBUG_STREAM(logger) << ctx.handler.client_identify() << ": "

#define CLIENT_INFO(logger, ctx) \
	LOG_INFO_STREAM(logger) << ctx.handler.client_identify() << ": "

#define CLIENT_WARN(logger, ctx) \
    LOG_WARN_STREAM(logger) << ctx.handler.client_identify() << ": "

#define CLIENT_ERROR(logger, ctx) \
	LOG_ERROR_STREAM(logger) << ctx.handler.client_identify() << ": "

#define CLIENT_FATAL(logger, ctx) \
	LOG_FATAL_STREAM(logger) << ctx.handler.client_identify() << ": "

#define CLIENT_TRACE_GLOB(ctx) \
	CLIENT_TRACE(ember::log::global_logger(), ctx)

#define CLIENT_DEBUG_GLOB(ctx) \
	CLIENT_DEBUG(ember::log::global_logger(), ctx)

#define CLIENT_INFO_GLOB(ctx) \
	CLIENT_INFO(ember::log::global_logger(), ctx)

#define CLIENT_WARN_GLOB(ctx) \
	CLIENT_WARN(ember::log::global_logger(), ctx)

#define CLIENT_ERROR_GLOB(ctx) \
	CLIENT_ERROR(ember::log::global_logger(), ctx)

#define CLIENT_FATAL_GLOB(ctx) \
	CLIENT_FATAL(ember::log::global_logger(), ctx)
