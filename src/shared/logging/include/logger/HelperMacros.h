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
	if(logger->severity() <= ember::log::SEVERITY::TRACE) { \
		*logger << ember::log::SEVERITY::TRACE

#define LOG_DEBUG(logger) \
	if(logger->severity() <= ember::log::SEVERITY::DEBUG) { \
		*logger << ember::log::SEVERITY::DEBUG

#define LOG_INFO(logger) \
	if(logger->severity() <= ember::log::SEVERITY::INFO) { \
		*logger << ember::log::SEVERITY::INFO

#define LOG_WARN(logger) \
	if(logger->severity() <= ember::log::SEVERITY::WARN) { \
		*logger << ember::log::SEVERITY::WARN

#define LOG_ERROR(logger) \
	if(logger->severity() <= ember::log::SEVERITY::ERROR) { \
		*logger << ember::log::SEVERITY::ERROR

#define LOG_FATAL(logger) \
	if(logger->severity() <= ember::log::SEVERITY::FATAL) { \
		*logger << ember::log::SEVERITY::FATAL

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

#define LOG_FLUSH ember::log::flush; }
#define LOG_SYNC ember::log::flush_sync; }