/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/ConsoleSink.h>
#include <logger/Exception.h>
#include <logger/Utility.h>
#include <shared/util/ConsoleColour.h>
#include <algorithm>
#include <iterator>
#include <string>
#include <cstdio>

namespace ember::log {

void ConsoleSink::batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	if(!colour_) {
		do_batch_write(records);
	} else { // we can't do batch output if we need to colour each individual log record
		for(auto& [detail, data] : records) {
			write(detail.severity, detail.type, data, false);
		}
	}
}

void ConsoleSink::do_batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	std::size_t size = 0;
	Severity sink_sev = this->severity();
	Filter sink_filter = this->filter();
	bool matches = false;

	for(auto&& [detail, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter &detail.type)) {
			size += data.size();
			matches = true;
		}
	}

	if(!matches) {
		return;
	}

	std::vector<char> buffer;
	buffer.reserve(size + (10 * records.size()));
	std::string severity;

	for(auto&& [detail, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter & detail.type)) {
			severity = detail::severity_string(detail.severity);
			std::copy(severity.begin(), severity.end(), std::back_inserter(buffer));
			std::copy(data.begin(), data.end(), std::back_inserter(buffer));
		}
	}

	std::fwrite(buffer.data(), buffer.size(), 1, stdout);
}

void ConsoleSink::write(Severity severity, Filter type, const std::vector<char>& record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	util::Colour old_colour;

	if(colour_) {
		old_colour = util::save_output_colour();
		set_colour(severity);
	}

	std::string buffer = detail::severity_string(severity);
	buffer.append(record.begin(), record.end());
	std::fwrite(buffer.c_str(), buffer.size(), 1, stdout);


	if(flush) {
		if(std::fflush(stdout) != 0) {
			throw exception("Unable to flush log record to console");
		}
	}

	if(colour_) {
		util::set_output_colour(old_colour);
	}
}

void ConsoleSink::set_colour(Severity severity) {
	switch(severity) {
		case Severity::FATAL:
			[[fallthrough]];
		case Severity::ERROR_:
			[[fallthrough]];
		case Severity::WARN:
			util::set_output_colour(util::Colour::LIGHT_RED);
			break;
		case Severity::INFO:
			util::set_output_colour(util::Colour::WHITE);
			break;
		case Severity::DEBUG:
			util::set_output_colour(util::Colour::LIGHT_CYAN);
			break;
		case Severity::TRACE:
			util::set_output_colour(util::Colour::DARK_GREY);
			break;
		case Severity::DISABLED:
			// shutting the compiler up
			break;
	}
}

} //log, ember