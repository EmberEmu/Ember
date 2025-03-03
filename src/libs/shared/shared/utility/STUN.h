/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Client.h>
#include <stun/Utility.h>
#include <logger/Logger.h>
#include <logger/HelperMacros.h>
#include <boost/program_options.hpp>
#include <format>
#include <memory>
#include <cstdint>

namespace po = boost::program_options;

namespace ember {

inline static stun::Client create_stun_client(const po::variables_map& args) {
	const auto& proto_arg = args["stun.protocol"].as<std::string>();

	if(proto_arg != "tcp" && proto_arg != "udp") {
		throw std::invalid_argument("Invalid STUN protocol argument");
	}

	return stun::Client(
		args["network.interface"].as<std::string>(),
		args["stun.server"].as<std::string>(),
		args["stun.port"].as<std::uint16_t>(),
		proto_arg == "tcp"? stun::Protocol::TCP : stun::Protocol::UDP
	);
}

inline static void stun_log_callback(stun::Verbosity verbosity, stun::Error reason, log::Logger& logger) {
	constexpr std::string_view fmt = "[stun] {}";
	const std::string_view reason_str = to_string(reason);

	switch(verbosity) {
		case stun::Verbosity::STUN_LOG_TRIVIAL:
			LOG_TRACE_SYNC(logger, fmt, reason_str);
			break;
		case stun::Verbosity::STUN_LOG_DEBUG:
			LOG_DEBUG_SYNC(logger, fmt, reason_str);
			break;
		case stun::Verbosity::STUN_LOG_INFO:
			LOG_INFO_SYNC(logger, fmt, reason_str);
			break;
		case stun::Verbosity::STUN_LOG_WARN:
			LOG_WARN_SYNC(logger, fmt, reason_str);
			break;
		case stun::Verbosity::STUN_LOG_ERROR:
			LOG_ERROR_SYNC(logger, fmt, reason_str);
			break;
		case stun::Verbosity::STUN_LOG_FATAL:
			LOG_FATAL_SYNC(logger, fmt, reason_str);
			break;
	}
}

inline void log_stun_result(stun::Client& client, const stun::MappedResult& result,
                            const std::uint16_t port, log::Logger& logger) {
	if(!result) {
		LOG_ERROR_SYNC(logger, "STUN: Query failed ({})", stun::to_string(result.error().reason));
		return;
	}

	const auto& ip = stun::extract_ip_to_string(*result);
	LOG_INFO_SYNC(logger, "STUN: Binding request succeeded ({})", ip);

	const auto nat = client.nat_present().get();

	if(!nat) {
		LOG_WARN_SYNC(logger, "STUN: Unable to determine if gateway is behind NAT ({})",
		                      stun::to_string(nat.error().reason));
		return;
	}

	if(*nat) {
		LOG_INFO_SYNC(logger, "STUN: Service appears to be behind NAT, "
		                      "forward port {} for external access", port);
	} else {
		LOG_INFO_SYNC(logger, "STUN: Service does not appear to be behind NAT - "
		                      "server is available online (firewall rules permitting)");
	}
}
} // ember