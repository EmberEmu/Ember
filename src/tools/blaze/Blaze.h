/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logger.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string_view>

namespace ember::blaze {

static inline constexpr cstring_view app_name { "Blaze" };

class Blaze {
	const boost::program_options::variables_map& opts_;
	log::Logger& logger_;

public:
	Blaze(const boost::program_options::variables_map& opts, log::Logger& logger);
	void run();
};

} // blaze, ember