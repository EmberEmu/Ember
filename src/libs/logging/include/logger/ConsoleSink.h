/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <cstdio>

namespace ember { namespace log {

class ConsoleSink : public Sink {
public:
	ConsoleSink(Severity severity, Filter filter) : Sink(severity, filter) {
		// LOG_SYNC guarantees that messages will be flushed before continuining
		// execution, so buffering needs to be disabled
		std::setbuf(stdout, nullptr);
	}

	void write(Severity severity, Filter type, const std::vector<char>& record) override;
	void batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) override;
};

}} //log, ember