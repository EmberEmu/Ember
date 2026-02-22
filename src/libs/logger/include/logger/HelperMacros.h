/*
* Copyright (c) 2015 - 2026 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <logger/Severity.h>

inline auto& log_deref(auto& x) { return x; }
inline auto& log_deref(auto* x) { return *x; }

#if !defined(NO_LOGGING) && !defined(NO_TRACE_LOGGING)
	#define LOG_TRACE(logger) \
		if(logger->severity() <= ember::log::Severity::trace) \
			log_deref(logger) << ember::log::Severity::trace
#else
	#define LOG_TRACE(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_DEBUG_LOGGING)
	#define LOG_DEBUG(logger) \
		if(logger->severity() <= ember::log::Severity::debug) \
			log_deref(logger) << ember::log::Severity::debug
#else
	#define LOG_DEBUG(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_INFO_LOGGING)
	#define LOG_INFO(logger) \
		if(logger->severity() <= ember::log::Severity::info) \
			log_deref(logger) << ember::log::Severity::info
#else
	#define LOG_INFO(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_WARN_LOGGING)
	#define LOG_WARN(logger) \
	if(logger->severity() <= ember::log::Severity::warn) \
			log_deref(logger) << ember::log::Severity::warn
#else
	#define LOG_WARN(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_ERROR_LOGGING)
	#define LOG_ERROR(logger) \
		if(logger->severity() <= ember::log::Severity::error) \
			log_deref(logger) << ember::log::Severity::error
#else
	#define LOG_ERROR(logger) \
		if(false) \
			log_deref(logger)
#endif


#if !defined(NO_LOGGING) && !defined(NO_FATAL_LOGGING)
	#define LOG_FATAL(logger) \
		if(logger->severity() <= ember::log::Severity::fatal) \
			log_deref(logger) << ember::log::Severity::fatal
#else
	#define LOG_FATAL(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE(logger) \
		if(logger->severity() <= ember::log::Severity::console) \
			log_deref(logger) << ember::log::Severity::console
#else
#define LOG_CONSOLE(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_ERR(logger) \
		if(logger->severity() <= ember::log::Severity::console_error) \
			log_deref(logger) << ember::log::Severity::console_error
#else
#define LOG_CONSOLE_ERR(logger) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_TRACE_LOGGING)
	#define LOG_TRACE_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::trace && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::trace << ember::log::Filter(type)
#else
	#define LOG_TRACE_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_DEBUG_LOGGING)
	#define LOG_DEBUG_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::debug && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::debug << ember::log::Filter(type)
#else
	#define LOG_DEBUG_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_INFO_LOGGING)
	#define LOG_INFO_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::info && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::info << ember::log::Filter(type)
#else
	#define LOG_INFO_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_WARN_LOGGING)
	#define LOG_WARN_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::warn && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::warn << ember::log::Filter(type)
#else
	#define LOG_WARN_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_ERROR_LOGGING)
	#define LOG_ERROR_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::error && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::error << ember::log::Filter(type)
#else
	#define LOG_ERROR_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif


#if !defined(NO_LOGGING) && !defined(NO_FATAL_LOGGING)
	#define LOG_FATAL_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::fatal && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::fatal << ember::log::Filter(type)
#else
	#define LOG_FATAL_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::console && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::console << ember::log::Filter(type)
#else
#define LOG_CONSOLE_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_ERROR_FILTER(logger, type) \
		if(logger->severity() <= ember::log::Severity::console_error && !(logger->filter() & type)) \
			log_deref(logger) << ember::log::Severity::console_error << ember::log::Filter(type)
#else
#define LOG_CONSOLE_ERROR_FILTER(logger, type) \
		if(false) \
			log_deref(logger)
#endif

#define LOG_TRACE_GLOB \
	LOG_TRACE(ember::log::global_logger())

#define LOG_DEBUG_GLOB \
	LOG_DEBUG(ember::log::global_logger())

#define LOG_INFO_GLOB \
	LOG_INFO(ember::log::global_logger())

#define LOG_WARN_GLOB \
	LOG_WARN(ember::log::global_logger())

#define LOG_ERROR_GLOB \
	LOG_ERROR(ember::log::global_logger())

#define LOG_FATAL_GLOB \
	LOG_FATAL(ember::log::global_logger())

#define LOG_CONSOLE_GLOB \
	LOG_CONSOLE(ember::log::global_logger())

#define LOG_TRACE_FILTER_GLOB(filter) \
	LOG_TRACE_FILTER(ember::log::global_logger(), filter)

#define LOG_DEBUG_FILTER_GLOB(filter) \
	LOG_DEBUG_FILTER(ember::log::global_logger(), filter)

#define LOG_INFO_FILTER_GLOB(filter) \
	LOG_INFO_FILTER(ember::log::global_logger(), filter)

#define LOG_WARN_FILTER_GLOB(filter) \
	LOG_WARN_FILTER(ember::log::global_logger(), filter)

#define LOG_ERROR_FILTER_GLOB(filter) \
	LOG_ERROR_FILTER(ember::log::global_logger(), filter)

#define LOG_FATAL_FILTER_GLOB(filter) \
	LOG_FATAL_FILTER(ember::log::global_logger(), filter)

#define LOG_CONSOLE_FILTER_GLOB(filter) \
	LOG_CONSOLE_FILTER(ember::log::global_logger(), filter)

#define LOG_CONSOLE_ERROR_FILTER_GLOB(filter) \
	LOG_CONSOLE_ERROR_FILTER_GLOB(ember::log::global_logger(), filter)

#define LOG_ASYNC ember::log::flush
#define LOG_SYNC  ember::log::flush_sync

#if !defined(NO_LOGGING) && !defined(NO_TRACE_LOGGING)
	#define LOG_TRACE_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::trace) \
			logger->fmt_write<true>(ember::log::Severity::trace, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_TRACE_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_DEBUG_LOGGING)
	#define LOG_DEBUG_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::debug) \
			logger->fmt_write<true>(ember::log::Severity::debug, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_DEBUG_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_INFO_LOGGING)
	#define LOG_INFO_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::info) \
			logger->fmt_write<true>(ember::log::Severity::info, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_INFO_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_WARN_LOGGING)
	#define LOG_WARN_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::warn) \
			logger->fmt_write<true>(ember::log::Severity::warn, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_WARN_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_ERROR_LOGGING)
	#define LOG_ERROR_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::error) \
			logger->fmt_write<true>(ember::log::Severity::error, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_ERROR_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_FATAL_LOGGING)
	#define LOG_FATAL_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::fatal) \
			logger->fmt_write<true>(ember::log::Severity::fatal, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_FATAL_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::console) \
			logger->fmt_write<true>(ember::log::Severity::console, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_CONSOLE_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_ERROR_ASYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::console_error) \
			logger->fmt_write<true>(ember::log::Severity::console_error, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_CONSOLE_ERROR_ASYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_TRACE_LOGGING)
	#define LOG_TRACE_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::trace) \
			logger->fmt_write<false>(ember::log::Severity::trace, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_TRACE_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_DEBUG_LOGGING)
	#define LOG_DEBUG_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::debug) \
			logger->fmt_write<false>(ember::log::Severity::debug, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_DEBUG_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_INFO_LOGGING)
	#define LOG_INFO_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::info) \
			logger->fmt_write<false>(ember::log::Severity::info, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_INFO_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_WARN_LOGGING)
	#define LOG_WARN_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::warn) \
			logger->fmt_write<false>(ember::log::Severity::warn, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_WARN_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_ERROR_LOGGING)
	#define LOG_ERROR_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::error) \
			logger->fmt_write<false>(ember::log::Severity::error, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_ERROR_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_FATAL_LOGGING)
	#define LOG_FATAL_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::fatal) \
			logger->fmt_write<false>(ember::log::Severity::fatal, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
	#define LOG_FATAL_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::console) \
			logger->fmt_write<false>(ember::log::Severity::console, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_CONSOLE_SYNC(logger, fmt_str, ...) \
		if(false);
#endif

#if !defined(NO_LOGGING) && !defined(NO_CONSOLE_LOGGING)
#define LOG_CONSOLE_ERROR_SYNC(logger, fmt_str, ...) \
		if(logger->severity() <= ember::log::Severity::console_error) \
			logger->fmt_write<false>(ember::log::Severity::console_error, fmt_str __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_CONSOLE_ERROR_SYNC(logger, fmt_str, ...) \
		if(false);
#endif


// used to generate decorated output (e.g. 'namespace::func' vs simply 'func')
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
	#define log_func __FUNCTION__
#elif defined(__clang__) || defined(__GNUC__)
	#define log_func __PRETTY_FUNCTION__
#else
	#define log_func __func__
#endif
