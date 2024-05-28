/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Sink.h"

namespace ember {

class ConsoleSink final : public Sink {
	inline static const char* time_fmt_ = "%Y-%m-%dT%H:%M:%SZ"; // ISO 8601, can be overriden by header

	void print_opcode(const fblog::Message& message) const;

public:
	void handle(const fblog::Header& header) override;
	void handle(const fblog::Message& message) override;
};

} // ember