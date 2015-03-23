/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Severity.h"

#define LOG_TRACE(logger) \
	if(logger->severity() <= ember::log::Severity::TRACE) { \
		*logger << ember::log::Severity::TRACE

#define LOG_DEBUG(logger) \
	if(logger->severity() <= ember::log::Severity::DEBUG) { \
		*logger << ember::log::Severity::DEBUG

#define LOG_INFO(logger) \
	if(logger->severity() <= ember::log::Severity::INFO) { \
		*logger << ember::log::Severity::INFO

#define LOG_WARN(logger) \
	if(logger->severity() <= ember::log::Severity::WARN) { \
		*logger << ember::log::Severity::WARN

#define LOG_ERROR(logger) \
	if(logger->severity() <= ember::log::Severity::ERROR) { \
		*logger << ember::log::Severity::ERROR

#define LOG_FATAL(logger) \
	if(logger->severity() <= ember::log::Severity::FATAL) { \
		*logger << ember::log::Severity::FATAL

#define LOG_TRACE_GLOB \
	auto logger = ember::log::get_logger(); \
	LOG_TRACE(logger)

#define LOG_DEBUG_GLOB \
	auto logger = ember::log::get_logger(); \
	LOG_DEBUG(logger)

#define LOG_INFO_GLOB \
	auto logger = ember::log::get_logger(); \
	LOG_INFO(logger)

#define LOG_WARN_GLOB \
	auto logger = ember::log::get_logger(); \
	LOG_WARN(logger)

#define LOG_ERROR_GLOB \
	auto logger = ember::log::get_logger(); \
	LOG_ERROR(logger)

#define LOG_FATAL_GLOB \
	auto logger = ember::log::get_logger(); \
	LOG_FATAL(logger)

#define LOG_ASYNC ember::log::flush; }
#define LOG_SYNC ember::log::flush_sync; }

#if _MSC_VER && !__INTEL_COMPILER //todo, VS2013 workaround, remove in VS2015, I hope
	#define __func__ __FUNCTION__
#endif