/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Plugin.h"
#include <commands/Command.h>
#include <logger/Logger.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options/variables_map.hpp>
#include <filesystem>
#include <string_view>
#include <vector>

namespace ember::blaze {

static inline constexpr cstring_view app_name { "Blaze" };

class Blaze {
	const boost::program_options::variables_map& args_;
	commands::Command& registry_;
	log::Logger& logger_;

	void start_services(boost::asio::io_context& ioc);
	void load_plugins();
	void load_plugin(const std::filesystem::path& path);

public:
	Blaze(const boost::program_options::variables_map& args, commands::Command& registry, log::Logger& logger);
	int run();
};

} // blaze, ember