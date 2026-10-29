/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LogSink.h"
#include "../FilterTypes.h"
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/FormatPacket.h>
#include <logger/Utility.h>
#include <array>
#include <format>
#include <memory>
#include <string_view>
#include <cstddef>

using namespace std::string_view_literals;

namespace ember::realm {

LogSink::LogSink(log::Logger& logger, log::Severity severity, std::string remote_host)
	: logger_(logger)
	, severity_(severity)
	, remote_host_(std::move(remote_host)) {
	start_log();
}

void LogSink::start_log() {
	logger_ << severity_ << log::Filter(lf_packet_log)
		<< "Starting packet logging for " << remote_host_ << log::flush;
}

void LogSink::log(std::span<const std::uint8_t> buffer, const std::time_t& time, PacketDirection dir) {
	const auto output = utility::format_packet(buffer.data(), buffer.size());

	cstring_view fmt("%H:%M:%S");
	std::tm tm;

#if _MSC_VER && !__INTEL_COMPILER
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif

	std::array<char, 64> time_buf{};
	const auto size = log::detail::put_time(tm, fmt, std::span(time_buf));

	if(!size) [[unlikely]] {
		throw std::runtime_error("timestamp formatting failed");
	}

	if(size >= time_buf.size()) [[unlikely]] {
		throw std::runtime_error("timestamp buffer out of space");
	}

	const std::string_view timestamp(time_buf.data(), size);
	const auto direction = dir == PacketDirection::inbound? "inbound"sv : "outbound"sv;
	auto msg = std::format("[{}] {}, {}:\n{}", timestamp, remote_host_, direction, output);
	logger_ << severity_ << log::Filter(lf_packet_log) << msg << log::flush;
}

LogSink::~LogSink() {
	logger_ << severity_ << log::Filter(lf_packet_log)
		<< "Ending packet logging for " << remote_host_ << log::flush;
}

} // realm, ember