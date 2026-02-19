/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>

namespace ember::log {

class SyslogSink final : public Sink {
	class impl;
	std::unique_ptr<impl> pimpl_;

public:	
	enum class Facility : std::uint8_t {
		kernel, user_level, mail_system, system_daemon,
		security_and_auth, syslogd_internal, line_printer_subsystem,
		network_news_subsystem, uucp_subsystem, clock_daemon,
		security_and_auth_2, ftp_daemon, ntp_daemon, log_audit,
		log_alert, clock_daemon_2, local_use_0, local_use_1,
		local_use_2, local_use_3, local_use_4, local_use_5,
		local_use_6, local_use_7
	};

	SyslogSink(log::Severity severity, Filter filter, std::string host, unsigned int port,
	           Facility facility, std::string tag);
	~SyslogSink();
	void write(log::Severity severity, Filter type, std::span<const char> record, bool flush) override;
	void batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records) override;
};

} // log, ember