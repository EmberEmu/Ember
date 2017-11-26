/*
 * Copyright (c) 2015, 2016 Ember
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

namespace ember::log {

class SyslogSink::impl : public Sink {
	enum class SyslogSeverity {
		EMERGENCY, ALERT, CRITICAL, ERROR_, WARNING, NOTICE, INFORMATIONAL, DEBUG
	};

	boost::asio::io_service service_;
	bai::udp::socket socket_;
	std::string host_;
	std::string tag_; 
	Facility facility_;

	std::string month_map(int month);
	SyslogSeverity severity_map(Severity severity);

public:
	impl(Severity severity, Filter filter, std::string host, unsigned int port, Facility facility, std::string tag);
	void write(Severity severity, Filter type, const std::vector<char>& record, bool flush);
	void batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& record);
};

SyslogSink::impl::impl(Severity severity, Filter filter, std::string host, unsigned int port,
                       Facility facility, std::string tag) try
                       : Sink(severity, filter), socket_(service_), host_(bai::host_name()), tag_(std::move(tag)) {
	facility_ = facility;

	if(tag.size() > 32) {
		throw exception("Syslog tag size must be 32 characters or less");
	}

	bai::udp::resolver resolver(service_);
	bai::udp::resolver::query query(host, std::to_string(port));
	boost::asio::connect(socket_, resolver.resolve(query));
} catch(std::exception& e) {
	throw exception(e.what());
}

auto SyslogSink::impl::severity_map(Severity severity) -> SyslogSeverity {
	switch(severity) {
		case Severity::FATAL:
			return SyslogSink::impl::SyslogSeverity::EMERGENCY;
		case Severity::ERROR_:
			return SyslogSink::impl::SyslogSeverity::ERROR_;
		case Severity::WARN:
			return SyslogSink::impl::SyslogSeverity::WARNING;
		case Severity::INFO:
			return SyslogSink::impl::SyslogSeverity::INFORMATIONAL;
		case Severity::DEBUG:
		case Severity::TRACE:
			return SyslogSink::impl::SyslogSeverity::DEBUG;
		default:
			BOOST_ASSERT_MSG(false, "SyslogSink encountered an unknown severity.");
			return SyslogSink::impl::SyslogSeverity::DEBUG;
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

void SyslogSink::impl::write(Severity severity, Filter type, const std::vector<char>& record, bool flush) {
	if(this->severity() >= severity || (this->filter() & type)) {
		return;
	}
	
	int priority_val = (static_cast<int>(facility_) * 8)
	                    + static_cast<std::uint8_t>(severity_map(severity));
	std::string priority = "<" + std::to_string(priority_val) + ">";

	std::tm time = detail::current_time();
	std::stringstream stream;
	stream << month_map(time.tm_mon) << ((time.tm_mday < 10)? "  " : " " ) << time.tm_mday
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
	socket_.send(segments, 0, err);
}

void SyslogSink::impl::batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	for(auto& [detail, data] : records) {
		write(detail.severity, detail.type, data, false);
	}
}

SyslogSink::SyslogSink(Severity severity, Filter filter, std::string host, unsigned int port,
                       Facility facility, std::string tag)
                       : Sink(severity, filter),
                         pimpl_(std::make_unique<impl>(severity, filter, host, port, facility, std::move(tag))) {}

SyslogSink::~SyslogSink() = default;

void SyslogSink::write(Severity severity, Filter type, const std::vector<char>& record, bool flush) {
	pimpl_->write(severity, type, record, flush);
}

void SyslogSink::batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	pimpl_->batch_write(records);
}

} //log, ember
