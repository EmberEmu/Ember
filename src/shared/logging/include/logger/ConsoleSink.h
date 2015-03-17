/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sink.h"

namespace ember { namespace log {

class ConsoleSink : public Sink {
public:
	ConsoleSink(Severity severity) : Sink(severity) {}
	void write(Severity severity, const std::vector<char>& record) override;
	void batch_write(const std::vector<std::pair<Severity, std::vector<char>>>& records) override;
};

}} //log, ember