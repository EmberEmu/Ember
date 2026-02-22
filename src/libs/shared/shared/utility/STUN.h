/*
 * Copyright (c) 2024 - 2026 Ember
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
#include <string>
#include <cstdint>

namespace po = boost::program_options;

namespace ember {

inline static stun::Client create_stun_client(const po::variables_map& args) {
	return stun::Client(
		args["network.interface"].as<std::string>(),
		args["stun.server"].as<std::string>(),
		args["stun.port"].as<std::uint16_t>(),
		args["stun.protocol"].as<stun::Protocol>()
	);
}

inline void log_stun_result(stun::Client& client,
                            const stun::MappedResult& result,
                            const std::uint16_t port,
                            log::Logger& logger) {
	if(!result) {
		LOG_ERROR_SYNC(logger, "[stun] Query failed ({})", stun::to_string(result.error().reason));
		return;
	}

	const auto& ip = stun::extract_ip_to_string(*result);
	LOG_INFO_SYNC(logger, "[stun] Binding request succeeded, external address is {}", ip);

	const auto nat = client.nat_present().get();

	if(!nat) {
		LOG_WARN_SYNC(logger, "[stun] Unable to determine if gateway is behind NAT ({})",
		                      stun::to_string(nat.error().reason));
		return;
	}

	if(*nat) {
		LOG_INFO_SYNC(logger, "[stun] Service appears to be behind NAT, "
		                      "forward port {} for external access", port);
	} else {
		LOG_INFO_SYNC(logger, "[stun] Service does not appear to be behind NAT - "
		                      "server is available online (firewall rules permitting)");
	}
}
} // ember