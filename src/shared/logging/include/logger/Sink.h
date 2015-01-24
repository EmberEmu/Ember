/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Severity.h"
#include <vector>

namespace ember { namespace log {

class Sink {
	SEVERITY severity_;

public:
	Sink(SEVERITY severity) {
		severity_ = severity;
	}

	SEVERITY severity() { return severity_; }
	void severity(SEVERITY severity) { severity_ = severity; }
	virtual void write(SEVERITY severity, const std::vector<char>& record) = 0;
	virtual void batch_write(const std::vector<std::pair<SEVERITY, std::vector<char>>>& records) = 0;
};

}} //log, ember