/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sink.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ember { namespace log {

class SyslogSink : public Sink {
	class impl;
	std::unique_ptr<impl> pimpl_;

public:	
	enum class Facility : std::uint8_t {
		KERNEL, USER_LEVEL, MAIL_SYSTEM, SYSTEM_DAEMON,
		SECURITY_AND_AUTH, SYSLOGD_INTERNAL, LINE_PRINTER_SUBSYSTEM,
		NETWORK_NEWS_SUBSYSTEM, UUCP_SUBSYSTEM, CLOCK_DAEMON,
		SECURITY_AND_AUTH_2, FTP_DAEMON, NTP_SUBSYSTEM, LOG_AUDIT,
		LOG_ALERT, CLOCK_DAEMON_2, LOCAL_USE_0, LOCAL_USE_1,
		LOCAL_USE_2, LOCAL_USE_3, LOCAL_USE_4, LOCAL_USE_5,
		LOCAL_USE_6, LOCAL_USE_7
	};

	SyslogSink(log::Severity severity, Filter filter, std::string host, unsigned int port,
	           Facility facility, std::string tag);
	~SyslogSink();
	void write(log::Severity severity, Filter type, const std::vector<char>& record) override final;
	void batch_write(const std::vector<std::pair<log::RecordDetail, std::vector<char>>>& records) override final;
};

}} //log, ember