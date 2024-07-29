/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Client.h>
#include <stun/Utility.h>
#include <logger/Logging.h>
#include <boost/program_options.hpp>
#include <memory>
#include <cstdint>

namespace po = boost::program_options;

namespace ember {

static std::unique_ptr<stun::Client> create_stun_client(const po::variables_map& args) {
	if(!args["stun.enabled"].as<bool>()) {
		return nullptr;
	}

	const auto& proto_arg = args["stun.protocol"].as<std::string>();

	if(proto_arg != "tcp" && proto_arg != "udp") {
		throw std::invalid_argument("Invalid STUN protocol argument");
	}

	auto stun = std::make_unique<stun::Client>(
		args["network.interface"].as<std::string>(),
		args["stun.server"].as<std::string>(),
		args["stun.port"].as<std::uint16_t>(),
		proto_arg == "tcp"? stun::Protocol::TCP : stun::Protocol::UDP
	);

	return stun;
}

void stun_log_callback(stun::Verbosity verbosity, stun::Error reason, log::Logger* logger) {
	switch(verbosity) {
		case stun::Verbosity::STUN_LOG_TRIVIAL:
			LOG_TRACE(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_DEBUG:
			LOG_DEBUG(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_INFO:
			LOG_INFO(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_WARN:
			LOG_WARN(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_ERROR:
			LOG_ERROR(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_FATAL:
			LOG_FATAL(logger) << "[stun] " << reason << LOG_SYNC;
			break;
	}
}

} // ember