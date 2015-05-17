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

#define LOG_TRACE_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::TRACE && (logger->filter() & type)) { \
		*logger << ember::log::Severity::TRACE << ember::log::Filter(type)

#define LOG_DEBUG_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::DEBUG && (logger->filter() & type)) { \
		*logger << ember::log::Severity::DEBUG << ember::log::Filter(type)

#define LOG_INFO_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::INFO && (logger->filter() & type)) { \
		*logger << ember::log::Severity::INFO << ember::log::Filter(type)

#define LOG_WARN_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::WARN && (logger->filter() & type)) { \
		*logger << ember::log::Severity::WARN << ember::log::Filter(type)

#define LOG_ERROR_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::ERROR && (logger->filter() & type)) { \
		*logger << ember::log::Severity::ERROR << ember::log::Filter(type)

#define LOG_FATAL_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::FATAL && (logger->filter() & type)) { \
		*logger << ember::log::Severity::FATAL << ember::log::Filter(type)

#define LOG_TRACE_GLOB \
	LOG_TRACE(ember::log::get_logger())

#define LOG_DEBUG_GLOB \
	LOG_DEBUG(ember::log::get_logger())

#define LOG_INFO_GLOB \
	LOG_INFO(ember::log::get_logger())

#define LOG_WARN_GLOB \
	LOG_WARN(ember::log::get_logger())

#define LOG_ERROR_GLOB \
	LOG_ERROR(ember::log::get_logger())

#define LOG_FATAL_GLOB \
	LOG_FATAL(ember::log::get_logger())

#define LOG_TRACE_FILTER_GLOB(filter) \
	LOG_TRACE_FILTER(ember::log::get_logger(), filter)

#define LOG_DEBUG_FILTER_GLOB(filter) \
	LOG_DEBUG_FILTER(ember::log::get_logger(), filter)

#define LOG_INFO_FILTER_GLOB(filter) \
	LOG_INFO_FILTER(ember::log::get_logger(), filter)

#define LOG_WARN_FILTER_GLOB(filter) \
	LOG_WARN_FILTER(ember::log::get_logger(), filter)

#define LOG_ERROR_FILTER_GLOB(filter) \
	LOG_ERROR_FILTER(ember::log::get_logger(), filter)

#define LOG_FATAL_FILTER_GLOB(filter) \
	LOG_FATAL_FILTER(ember::log::get_logger(), filter)

#define LOG_ASYNC ember::log::flush; }
#define LOG_SYNC  ember::log::flush_sync; }

#if _MSC_VER && !__INTEL_COMPILER //todo, VS2013 workaround, remove in VS2015, I hope
	#define __func__ __FUNCTION__
#endif