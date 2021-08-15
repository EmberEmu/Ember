/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>

#if !NO_LOGGING && !NO_TRACE_LOGGING
#define LOG_TRACE(logger) \
	if(logger->severity() <= ember::log::Severity::TRACE) { \
		*logger << ember::log::Severity::TRACE
#else
#define LOG_TRACE(logger) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_DEBUG_LOGGING
#define LOG_DEBUG(logger) \
	if(logger->severity() <= ember::log::Severity::DEBUG) { \
		*logger << ember::log::Severity::DEBUG
#else
#define LOG_DEBUG(logger) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_INFO_LOGGING
#define LOG_INFO(logger) \
	if(logger->severity() <= ember::log::Severity::INFO) { \
		*logger << ember::log::Severity::INFO
#else
#define LOG_INFO(logger) \
	if(false) {	\
		*logger
#endif

#if !NO_LOGGING && !NO_WARN_LOGGING
#define LOG_WARN(logger) \
if(logger->severity() <= ember::log::Severity::WARN) { \
		*logger << ember::log::Severity::WARN
#else
#define LOG_WARN(logger) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_ERROR_LOGGING
#define LOG_ERROR(logger) \
	if(logger->severity() <= ember::log::Severity::ERROR_) { \
		*logger << ember::log::Severity::ERROR_
#else
#define LOG_ERROR(logger) \
	if(false) { \
		*logger
#endif


#if !NO_LOGGING && !NO_FATAL_LOGGING
#define LOG_FATAL(logger) \
	if(logger->severity() <= ember::log::Severity::FATAL) { \
		*logger << ember::log::Severity::FATAL
#else
#define LOG_FATAL(logger) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_TRACE_LOGGING
#define LOG_TRACE_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::TRACE && !(logger->filter() & type)) { \
		*logger << ember::log::Severity::TRACE << ember::log::Filter(type)
#else
#define LOG_TRACE_FILTER(logger, type) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_DEBUG_LOGGING
#define LOG_DEBUG_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::DEBUG && !(logger->filter() & type)) { \
		*logger << ember::log::Severity::DEBUG << ember::log::Filter(type)
#else
#define LOG_DEBUG_FILTER(logger, type) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_INFO_LOGGING
#define LOG_INFO_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::INFO && !(logger->filter() & type)) { \
		*logger << ember::log::Severity::INFO << ember::log::Filter(type)
#else
#define LOG_INFO_FILTER(logger, type) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_WARN_LOGGING
#define LOG_WARN_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::WARN && !(logger->filter() & type)) { \
		*logger << ember::log::Severity::WARN << ember::log::Filter(type)
#else
#define LOG_WARN_FILTER(logger, type) \
	if(false) { \
		*logger
#endif

#if !NO_LOGGING && !NO_ERROR_LOGGING
#define LOG_ERROR_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::ERROR_ && !(logger->filter() & type)) { \
		*logger << ember::log::Severity::ERROR_ << ember::log::Filter(type)
#else
#define LOG_ERROR_FILTER(logger, type) \
	if(false) { \
		*logger
#endif


#if !NO_LOGGING && !NO_FATAL_LOGGING
#define LOG_FATAL_FILTER(logger, type) \
	if(logger->severity() <= ember::log::Severity::FATAL && !(logger->filter() & type)) { \
		*logger << ember::log::Severity::FATAL << ember::log::Filter(type)
#else
#define LOG_FATAL_FILTER(logger, type) \
	if(false) \
		*logger
#endif

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

#if !NO_LOGGING && !NO_TRACE_LOGGING
#define LOG_TRACE_FMT(logger, fmt_str, ...) \
	if(logger->severity() <= ember::log::Severity::TRACE) \
		logger->fmt_write(ember::log::Severity::TRACE, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_TRACE_FMT(logger, fmt_str, ...) \
	if(false);
#endif

#if !NO_LOGGING && !NO_DEBUG_LOGGING
#define LOG_DEBUG_FMT(logger, fmt_str, ...) \
	if(logger->severity() <= ember::log::Severity::DEBUG) \
		logger->fmt_write(ember::log::Severity::DEBUG, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_DEBUG_FMT(logger, fmt_str, ...) \
	if(false);
#endif

#if !NO_LOGGING && !NO_INFO_LOGGING
#define LOG_INFO_FMT(logger, fmt_str, ...) \
	if(logger->severity() <= ember::log::Severity::INFO) \
		logger->fmt_write(ember::log::Severity::INFO, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_INFO_FMT(logger, fmt_str, ...) \
	if(false);
#endif

#if !NO_LOGGING && !NO_WARN_LOGGING
#define LOG_WARN_FMT(logger, fmt_str, ...) \
	if(logger->severity() <= ember::log::Severity::WARN) \
		logger->fmt_write(ember::log::Severity::WARN, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_WARN_FMT(logger, fmt_str, ...) \
	if(false);
#endif

#if !NO_LOGGING && !NO_ERROR_LOGGING
#define LOG_ERROR_FMT(logger, fmt_str, ...) \
	if(logger->severity() <= ember::log::Severity::ERROR_) \
		logger->fmt_write(ember::log::Severity::ERROR_, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_ERROR_FMT(logger, fmt_str, ...) \
	if(false);
#endif

#if !NO_LOGGING && !NO_FATAL_LOGGING
#define LOG_FATAL_FMT(logger, fmt_str, ...) \
	if(logger->severity() <= ember::log::Severity::FATAL) \
		logger->fmt_write(ember::log::Severity::FATAL, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_FATAL_FMT(logger, fmt_str, ...) \
	if(false);
#endif

// used to generate decorated output (e.g. 'namespace::func' vs simply 'func')
#if _MSC_VER && !__INTEL_COMPILER
	#define __func__ __FUNCTION__
#elif __clang__ || __GNUC__
	#define __func__ __PRETTY_FUNCTION__
#endif
