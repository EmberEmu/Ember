/*
 * Copyright (c) 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>

#define CLIENT_TRACE(logger, ctx) \
	LOG_TRACE(logger) << ctx.handler->client_identify()

#define CLIENT_DEBUG(logger, ctx) \
	LOG_DEBUG(logger) << ctx.handler->client_identify()

#define CLIENT_INFO(logger, ctx) \
	LOG_INFO(logger) << ctx.handler->client_identify()

#define CLIENT_WARN(logger, ctx) \
    LOG_WARN(logger) << ctx.handler->client_identify()

#define CLIENT_ERROR(logger, ctx) \
	LOG_ERROR(logger) << ctx.handler->client_identify()

#define CLIENT_FATAL(logger, ctx) \
	LOG_FATAL(logger) << ctx.handler->client_identify()

#define CLIENT_TRACE_FILTER(logger, ctx, type) \
    LOG_TRACE_FILTER(logger, type) << ctx.handler->client_identify()

#define CLIENT_DEBUG_FILTER(logger, ctx, type) \
	LOG_DEBUG_FILTER(logger, type) << ctx.handler->client_identify()

#define CLIENT_INFO_FILTER(logger, ctx, type) \
	LOG_INFO_FILTER(logger, type) << ctx.handler->client_identify()

#define CLIENT_WARN_FILTER(logger, ctx, type) \
	LOG_WARN_FILTER(logger, type) << ctx.handler->client_identify()

#define CLIENT_ERROR_FILTER(logger, ctx, type) \
	LOG_ERROR_FILTER(logger, type) << ctx.handler->client_identify()

#define CLIENT_FATAL_FILTER(logger, ctx, type) \
	LOG_FATAL_FILTER(logger, type) << ctx.handler->client_identify()

#define CLIENT_TRACE_GLOB(ctx) \
	CLIENT_TRACE(ember::log::get_logger(), ctx)

#define CLIENT_DEBUG_GLOB(ctx) \
	CLIENT_DEBUG(ember::log::get_logger(), ctx)

#define CLIENT_INFO_GLOB(ctx) \
	CLIENT_INFO(ember::log::get_logger(), ctx)

#define CLIENT_WARN_GLOB(ctx) \
	CLIENT_WARN(ember::log::get_logger(), ctx)

#define CLIENT_ERROR_GLOB(ctx) \
	CLIENT_ERROR(ember::log::get_logger(), ctx)

#define CLIENT_FATAL_GLOB(ctx) \
	CLIENT_FATAL(ember::log::get_logger(), ctx)

#define CLIENT_TRACE_FILTER_GLOB(ctx, filter) \
	CLIENT_TRACE_FILTER(ember::log::get_logger(), ctx, filter)

#define CLIENT_DEBUG_FILTER_GLOB(ctx, filter) \
	CLIENT_DEBUG_FILTER(ember::log::get_logger(), ctx, filter)

#define CLIENT_INFO_FILTER_GLOB(ctx, filter) \
	CLIENT_INFO_FILTER(ember::log::get_logger(), ctx, filter)

#define CLIENT_WARN_FILTER_GLOB(ctx, filter) \
	CLIENT_WARN_FILTER(ember::log::get_logger(), ctx, filter)

#define CLIENT_ERROR_FILTER_GLOB(ctx, filter) \
	CLIENT_ERROR_FILTER(ember::log::get_logger(), ctx, filter)

#define CLIENT_FATAL_FILTER_GLOB(ctx, filter) \
	CLIENT_FATAL_FILTER(ember::log::get_logger(), ctx, filter)
