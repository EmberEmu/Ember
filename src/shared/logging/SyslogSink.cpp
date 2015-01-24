/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define _CRT_SECURE_NO_DEPRECATE
#include <logger/SyslogSink.h>
#include <logger/Utility.h>
#include <logger/Exception.h>
#include <boost/asio.hpp>
#include <boost/assert.hpp>

namespace bai = boost::asio::ip;

#undef ERROR

namespace ember { namespace log {

class SyslogSink::impl {
	enum class SYSLOG_SEVERITY {
		EMERGENCY, ALERT, CRITICAL, ERROR_, WARNING, NOTICE, INFORMATIONAL, DEBUG
	};

	boost::asio::io_service service_;
	bai::udp::socket socket_;
	bai::udp::endpoint endpoint_;
	SEVERITY severity_;
	std::string host_;
	std::string tag_;
	FACILITY facility_;

	std::string month_map(int month);
	SYSLOG_SEVERITY severity_map(SEVERITY severity);

public:
	impl(SEVERITY severity, std::string host, unsigned int port, FACILITY facility, std::string tag);
	void write(SEVERITY severity, const std::vector<char>& record);
	void batch_write(const std::vector<std::pair<SEVERITY, std::vector<char>>>& record);
};

SyslogSink::impl::impl(SEVERITY severity, std::string host, unsigned int port,
                       FACILITY facility, std::string tag) try
                       : socket_(service_), host_(bai::host_name()), tag_(tag) {
	facility_ = facility;

	if(tag.size() > 32) {
		throw exception("Syslog tag size must be 32 characters or less");
	}

	bai::udp::resolver resolver(service_);
	bai::udp::resolver::query query(host, std::to_string(port));
	endpoint_ = resolver.resolve(query)->endpoint();
	socket_.open(endpoint_.protocol());
} catch(std::exception& e) {
	throw exception(e.what());
}

SyslogSink::impl::SYSLOG_SEVERITY SyslogSink::impl::severity_map(SEVERITY severity) {
	switch(severity) {
		case SEVERITY::FATAL:
			return SyslogSink::impl::SYSLOG_SEVERITY::EMERGENCY;
		case SEVERITY::ERROR:
			return SyslogSink::impl::SYSLOG_SEVERITY::ERROR_;
		case SEVERITY::WARN:
			return SyslogSink::impl::SYSLOG_SEVERITY::WARNING;
		case SEVERITY::INFO:
			return SyslogSink::impl::SYSLOG_SEVERITY::INFORMATIONAL;
		case SEVERITY::DEBUG:
		case SEVERITY::TRACE:
			return SyslogSink::impl::SYSLOG_SEVERITY::DEBUG;
		default:
			BOOST_ASSERT_MSG(false, "SyslogSink encountered an unknown severity.");
			return SyslogSink::impl::SYSLOG_SEVERITY::DEBUG;
	}
}

std::string SyslogSink::impl::month_map(int month) {
	switch(month) {
		case 0: return "Jan";
		case 1: return "Feb";
		case 2: return "Mar";
		case 3: return "Apr";
		case 4: return "May";
		case 5: return "Jun";
		case 6: return "Jul";
		case 7: return "Aug";
		case 8: return "Sept";
		case 9: return "Oct";
		case 10: return "Nov";
		case 11: return "Dec";
		default: BOOST_ASSERT_MSG(false,
		"SyslogSink encountered an unknown month value. Only Earth is supported.");
			return "err";
	}
}

void SyslogSink::impl::write(SEVERITY severity, const std::vector<char>& record) {
	if(severity < severity_) {
		return;
	}
	
	int priority_val = (static_cast<int>(facility_) * 8)
	                    + static_cast<std::uint8_t>(severity_map(severity));
	std::string priority = "<" + std::to_string(priority_val) + ">";

	std::tm time = detail::current_time();
	std::stringstream stream;
	stream << std::string("Jan") << ((time.tm_mday < 10)? "  " : " " ) << time.tm_mday
	       << " " << time.tm_hour << ":" << time.tm_min << ":" << ((time.tm_sec < 10) ? "0" : "")
	       << time.tm_sec << " " << host_ << " ";

	const std::string& header = stream.str();
	std::vector<boost::asio::const_buffer> segments;
	segments.emplace_back(priority.data(), priority.size());
	segments.emplace_back(header.data(), header.size());
	segments.emplace_back(tag_.data(), tag_.size());
	segments.emplace_back(": ", 2);
	segments.emplace_back(record.data(), record.size());

	boost::system::error_code err; //ignoring any send errors
	socket_.send_to(segments, endpoint_, 0, err);
}

void SyslogSink::impl::batch_write(const std::vector<std::pair<SEVERITY, std::vector<char>>>& records) {
	for(auto& r : records) {
		write(r.first, r.second);
	}
}

SyslogSink::SyslogSink(SEVERITY severity, std::string host, unsigned int port,
                       FACILITY facility, std::string tag)
                       : Sink(severity),
                         pimpl_(std::make_unique<impl>(severity, host, port, facility, tag)) {}

SyslogSink::~SyslogSink() = default;

void SyslogSink::write(SEVERITY severity, const std::vector<char>& record) {
	pimpl_->write(severity, record);
}

void SyslogSink::batch_write(const std::vector<std::pair<SEVERITY, std::vector<char>>>& records) {
	pimpl_->batch_write(records);
}

}} //log, ember