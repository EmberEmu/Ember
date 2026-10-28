/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LogConfig.h"
#include <logger/Logger.h>
#ifdef _WIN32
#include <logger/win32/CommandSink.h>
#endif
#include <logger/posix/CommandSink.h>
#include <logger/ConsoleSink.h>
#include <logger/FileSink.h>
#include <logger/SyslogSink.h>
#include <string>
#include <stdexcept>
#include <utility>
#include <cstdint>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <cstdio>

namespace opts = boost::program_options;

namespace ember::utility {

namespace {

std::shared_ptr<log::Sink> init_remote_sink(const opts::variables_map& args, log::Severity severity) {
	const auto& host = args["remote_log.host"].as<std::string>();
	const auto& service = args["remote_log.service_name"].as<std::string>();
	auto port = args["remote_log.port"].as<std::uint16_t>();
	auto facility = log::SyslogSink::Facility::local_use_0;
	auto filter = args["remote_log.filter_mask"].as<std::uint32_t>();

	return std::make_shared<log::SyslogSink>(
		"remote_sink_0", severity, log::Filter(filter), host, port, facility, service
	);
}

std::shared_ptr<log::Sink> init_file_sink(const opts::variables_map& args, log::Severity severity) {
	const auto& mode_str = args["file_log.mode"].as<std::string>();
	const auto& path = args["file_log.path"].as<std::string>();
	auto filter = args["file_log.filter_mask"].as<std::uint32_t>();

	if(mode_str != "append" && mode_str != "truncate") {
		throw std::runtime_error("Invalid file logging mode supplied");
	}

	auto mode = (mode_str == "append")?
		log::FileSink::Mode::append : log::FileSink::Mode::truncate;

	auto sink = std::make_shared<log::FileSink>(
		"file_sink_0", severity, log::Filter(filter), path, mode
	);

	sink->size_limit( args["file_log.size_rotate"].as<std::uint32_t>());
	sink->log_severity(args["file_log.severity"].as<bool>());
	sink->log_date(args["file_log.timestamp"].as<bool>());
	sink->time_format(args["file_log.timestamp_format"].as<std::string>());
	sink->midnight_rotate(args["file_log.midnight_rotate"].as<bool>());
	return sink;
}

std::shared_ptr<log::Sink> init_console_sink(const opts::variables_map& args, log::Severity severity) {
	log::ConsoleSinkOptions options {
		.name = "console_sink_0",
		.severity = severity,
		.filter = log::Filter(args["console_log.filter_mask"].as<std::uint32_t>()),
		.colours = args["console_log.colours"].as<bool>(),
		.timestamp = args["console_log.timestamp"].as<bool>(),
		.timestamp_fmt = args["console_log.timestamp_format"].as<std::string>()
	};

	if(args.contains("console_log.prefix_width")) {
		options.prefix_width = args["console_log.prefix_width"].as<unsigned int>();
	}

	return std::make_shared<log::ConsoleSink>(std::move(options));
}

std::shared_ptr<log::posix::CommandSink> init_posix_command_sink(const opts::variables_map& args, log::Severity severity) {
	log::posix::CommandSinkOptions options {
		.prompt = "ember( ",
		.severity = severity,
		.filter = log::Filter(args["console_log.filter_mask"].as<std::uint32_t>()),
		.colours = args["console_log.colours"].as<bool>(),
		.timestamp = args["console_log.timestamp"].as<bool>(),
		.timestamp_fmt = args["console_log.timestamp_format"].as<std::string>(),
		.no_background = args["console_log.no_bg"].as<bool>(),
		.theme = args["console_log.theme"].as<log::posix::Theme>(),
	};

	if(args.contains("console_log.prefix_width")) {
		options.prefix_width = args["console_log.prefix_width"].as<unsigned int>();
	}

	return std::make_shared<log::posix::CommandSink>(std::move(options));
}

#ifdef _WIN32
std::shared_ptr<log::win32::CommandSink> init_win_command_sink(const opts::variables_map& args, log::Severity severity) {
		log::win32::CommandSinkOptions options {
		.prompt = "ember( ",
		.severity = severity,
		.filter = log::Filter(args["console_log.filter_mask"].as<std::uint32_t>()),
		.colours = args["console_log.colours"].as<bool>(),
		.timestamp = args["console_log.timestamp"].as<bool>(),
		.timestamp_fmt = args["console_log.timestamp_format"].as<std::string>()
	};

	if(args.contains("console_log.prefix_width")) {
		options.prefix_width = args["console_log.prefix_width"].as<unsigned int>();
	}

	return std::make_shared<log::win32::CommandSink>(std::move(options));
}
#endif

std::shared_ptr<log::Sink> init_command_sink(const opts::variables_map& args, log::Severity severity) {
	const auto filter = args["console_log.filter_mask"].as<std::uint32_t>();
	const auto colourise = args["console_log.colours"].as<bool>();
	unsigned int prefix_width = 0;

	if(args.contains("console_log.prefix_width")) {
		prefix_width = args["console_log.prefix_width"].as<unsigned int>();
	}

	std::shared_ptr<log::CommandSink> sink;

#ifdef _WIN32
	const auto ansi = args["console_log.win_ansi"].as<bool>();

	if(ansi) {
		try {
#endif
			sink = init_posix_command_sink(args, severity);
#ifdef _WIN32
		} catch(std::exception&) {
			// fall back to WinAPI impl. if we're running in a terminal that doesn't support ANSI VT
			sink = init_win_command_sink(args, severity);
		}
	} else {
		sink = init_win_command_sink(args, severity);
	}

#endif
	return sink;
}

bool isatty() {
#ifdef _WIN32
	return !!_isatty(_fileno(stdout));
#else
	return !!::isatty(fileno(stdout));
#endif
}

} // unnamed

void configure_logger(log::Logger& logger, const opts::variables_map& args) {
	const bool enable_input = args["console_log.enable_input"].as<bool>();
	auto severity = args["console_log.verbosity"].as<log::Severity>();

	if(severity != log::Severity::disabled) {
		if(enable_input && isatty()) { // fall back if we're redirected
			logger.add_sink(init_command_sink(args, severity));
		} else {
			logger.add_sink(init_console_sink(args, severity));
		}
	}

	// file logger
	severity = args["file_log.verbosity"].as<log::Severity>();

	if(severity != log::Severity::disabled) {
		logger.add_sink(init_file_sink(args, severity));
	}

	// remote logger
	severity = args["remote_log.verbosity"].as<log::Severity>();

	if(severity != log::Severity::disabled) {
		logger.add_sink(init_remote_sink(args, severity));
	}
}

} // utility, ember