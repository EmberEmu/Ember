/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>

namespace ember { namespace log {

class ConsoleSink final : public Sink {
public:
	ConsoleSink(Severity severity, Filter filter) : Sink(severity, filter) {}
	void write(Severity severity, Filter type, const std::vector<char>& record) override;
	void batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) override;
};

}} //log, ember