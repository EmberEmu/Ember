/*
 * Copyright (c) 2018 - 2025 Ember
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
#include <memory>
#include <string_view>
#include <cstddef>

namespace ember::gateway {

LogSink::LogSink(log::Logger& logger, log::Severity severity, std::string remote_host)
                 : logger_(logger), severity_(severity), remote_host_(std::move(remote_host)) {
	start_log();
}

void LogSink::start_log() {
	logger_ << severity_ << log::Filter(lf_packet_log)
		<< "Starting packet logging for " << remote_host_ << log::flush;
}

void LogSink::log(std::span<const std::uint8_t> buffer, const std::time_t& time,
                  PacketDirection dir) {
	const auto output = utility::format_packet(buffer.data(), buffer.size());

	cstring_view fmt("%H:%M:%S");
	std::string_view direction = dir == PacketDirection::inbound? "inbound" : "outbound";
	std::tm tm;

#if _MSC_VER && !__INTEL_COMPILER
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif

	std::array<char, 32> time_buf{};
	log::detail::put_time(tm, fmt, std::span(time_buf));

	logger_ << severity_ << log::Filter(lf_packet_log)
		<< "[" << time_buf.data() << "] " << remote_host_ << ", " << direction
		<< ":\n" << output << log::flush;
}

LogSink::~LogSink() {
	logger_ << severity_ << log::Filter(lf_packet_log)
		<< "Ending packet logging for " << remote_host_ << log::flush;
}

} // gateway, ember