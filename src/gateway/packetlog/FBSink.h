/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketSink.h"
#include <fstream>

namespace ember {

class FBSink final : public PacketSink {
	std::ofstream file_;
	const std::string time_fmt_ = "%Y-%m-%dT%H:%M:%SZ"; // ISO 8601

	void start_log(const std::string& filename, const std::string& host,
	               const std::string& remote_host);

public:
	FBSink(const std::string& filename, const std::string& host, const std::string& remote_host);

	void log(const spark::Buffer& buffer, std::size_t length, const std::time_t& time,
	         PacketDirection dir) override;
};

} // ember