/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/ConsoleSink.h>
#include <logger/Utility.h>
#include <string>

namespace ember { namespace log {

void ConsoleSink::batch_write(const std::vector<std::pair<Severity, std::vector<char>>>& records) {
	std::size_t size = 0;
	Severity sink_sev= this->severity();
	bool matches = false;

	for(auto& r : records) {
		if(sink_sev <= r.first) {
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
		severity = detail::severity_string(r.first);
		std::copy(severity.begin(), severity.end(), std::back_inserter(buffer));
		std::copy(r.second.begin(), r.second.end(), std::back_inserter(buffer));
	}

	fwrite(buffer.data(), buffer.size(), 1, stdout);
}

void ConsoleSink::write(Severity severity, const std::vector<char>& record) {
	if(this->severity() > severity) {
		return;
	}

	std::string buffer = detail::severity_string(severity);
	buffer.append(record.begin(), record.end());
	fwrite(buffer.c_str(), buffer.size(), 1, stdout);
}

}} //log, ember