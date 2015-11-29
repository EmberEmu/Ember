/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/ConsoleSink.h>
#include <logger/Utility.h>
#include <shared/rlutil.h>
#include <algorithm>
#include <iterator>
#include <string>
#include <cstdio>

namespace ember { namespace log {

void ConsoleSink::batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	if(!colour_) {
		do_batch_write(records);
	} else { // we can't do batch output if we need to colour each individual log record
		for(auto& r : records) {
			write(r.first.severity, r.first.type, r.second);
		}
	}
}

void ConsoleSink::do_batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	std::size_t size = 0;
	Severity sink_sev = this->severity();
	Filter sink_filter = this->filter();
	bool matches = false;

	for(auto& r : records) {
		if(sink_sev <= r.first.severity && !(sink_filter & r.first.type)) {
			size += r.second.size();
			matches = true;
		}
	}

	if(!matches) {
		return;
	}

	std::vector<char> buffer;
	buffer.reserve(size + (10 * records.size()));
	std::string severity;

	for(auto& r : records) {
		if(sink_sev <= r.first.severity && !(sink_filter & r.first.type)) {
			severity = detail::severity_string(r.first.severity);
			std::copy(severity.begin(), severity.end(), std::back_inserter(buffer));
			std::copy(r.second.begin(), r.second.end(), std::back_inserter(buffer));
		}
	}

	std::fwrite(buffer.data(), buffer.size(), 1, stdout);
}

void ConsoleSink::write(Severity severity, Filter type, const std::vector<char>& record) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	if(colour_) {
		set_colour(severity);
	}

	std::string buffer = detail::severity_string(severity);
	buffer.append(record.begin(), record.end());
	std::fwrite(buffer.c_str(), buffer.size(), 1, stdout);
}

#undef ERROR // todo, replace rlutil so ERROR doesn't leak from Windows.h

void ConsoleSink::set_colour(Severity severity) {
	switch(severity) {
		case Severity::FATAL:
		case Severity::ERROR:
		case Severity::WARN:
			rlutil::setColor(rlutil::LIGHTRED);
			break;
		case Severity::INFO:
			rlutil::setColor(rlutil::WHITE);
			break;
		case Severity::DEBUG:
			rlutil::setColor(rlutil::LIGHTCYAN);
			break;
		case Severity::TRACE:
			rlutil::setColor(rlutil::DARKGREY);
			break;
	}
}

}} //log, ember