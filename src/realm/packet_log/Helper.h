/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketLogger.h"
#include "FlatbuffersSink.h"
#include "LogSink.h"
#include <logger/Severity.h>
#include <memory>
#include <string>

namespace ember::realm {

// just a utility function to keep the creation logic out of the way - not very
// configurable but it doesn't really need to be
inline std::unique_ptr<PacketLogger> create_packet_logger(std::string realm, std::string remote,
                                                          std::string file, log::Logger& logger,
                                                          bool file_only) try {
	// these can throw - they succeed or fail together
	auto plogger = std::make_unique<PacketLogger>();
	auto sink_fb = std::make_unique<FlatbuffersSink>(std::move(file), std::move(realm), remote);

	if(!file_only) {
		auto sink_log = std::make_unique<LogSink>(logger, log::Severity::debug, std::move(remote));
		plogger->add_sink(std::move(sink_log));
	}

	plogger->add_sink(std::move(sink_fb));
	return plogger;
} catch(std::exception& e) {
	return nullptr;
}

} // realm, ember