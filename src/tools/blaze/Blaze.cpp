/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Blaze.h"
#include <thread/Utility.h>
#include <boost/asio/io_context.hpp>
#include <angelscript.h>
#include <thread>
#include <vector>

namespace ember::blaze {

Blaze::Blaze(const boost::program_options::variables_map& args, log::Logger& logger)
	: args_(args)
	, logger_(logger) {}

int Blaze::run() try {
	auto concurrency = args_["misc.threads"].as<unsigned int>();

	if(!concurrency) {
		concurrency = thread::hardware_concurrency([&](auto msg) {
			SLOG_ERROR(logger_, msg);
		});
	}

	boost::asio::io_context ioc(concurrency);
	start_services(ioc);

	std::vector<std::jthread> threads;
	threads.reserve(concurrency);

	for(unsigned int i = 1; i < concurrency; ++i) {
		threads.emplace_back(&boost::asio::io_context::run, &ioc);
		thread::set_name(threads[i], "Asio Worker");
	}

	ioc.run();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	SLOG_FATAL(logger_, e.what());
	return EXIT_FAILURE;
}

void Blaze::start_services(boost::asio::io_context& ioc) {

}

} // blaze, ember