/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Blaze.h"
#include <angelscript.h>

namespace ember::blaze {

Blaze::Blaze(const boost::program_options::variables_map& opts, log::Logger& logger)
	: opts_(opts)
	, logger_(logger) {}

void Blaze::run() {
	SLOG_INFO(logger_, "Running but not actually doing anything yet :)");
}

} // blaze, ember