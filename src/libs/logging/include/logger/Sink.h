/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>
#include <utility>
#include <vector>

namespace ember::log {

class Sink {
	Severity severity_;
	Filter filter_;

protected:
	Sink(Severity severity, Filter filter) : severity_(severity), filter_(filter) {}

public:
	Severity severity() const { return severity_; }
	Filter filter() const { return filter_; }
	void severity(Severity severity) { severity_ = severity; }
	void filter(const Filter& filter) { filter_ = filter; }
	virtual void write(Severity severity, Filter type, const std::vector<char>& record, bool flush) = 0;
	virtual void batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) = 0;
	virtual ~Sink() = default;
};

} //log, ember