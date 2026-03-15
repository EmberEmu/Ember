/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/LoggerFwd.h>
#include <commands/Registry.h>
#include <service/Service.h>
#include <shared/metrics/Metrics.h>
#include <shared/utility/cstring_view.hpp>
#include <thread/Utility.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <semaphore>

namespace ember::login {

constexpr cstring_view app_name { "Login Daemon" };

class EMBER_EXPORT_SERVICE Service final : public IService {
	boost::asio::io_context service;
	boost::asio::io_context::strand serialise;
	std::exception_ptr eptr;
	std::binary_semaphore stop_flag { 0 };
	std::atomic_bool stopping_;

	log::Logger& logger;
	commands::Registry& registry;
	std::chrono::steady_clock::time_point start_time;

	std::unique_ptr<Metrics> start_metrics(
		boost::asio::io_context& service,
		const boost::program_options::variables_map& args
	);

	void launch(const boost::program_options::variables_map& args, boost::asio::io_context& service);

public:
	static boost::program_options::options_description options();

	Service(log::Logger& logger, commands::Registry& registry);
	~Service();

	int run(const boost::program_options::variables_map& args);
	void stop();

	static std::unique_ptr<Service> create(log::Logger& logger, commands::Registry& registry) {
		return std::make_unique<Service>(logger, registry);
	}
};

extern "C" EMBER_EXPORT_SERVICE Service* create_login(log::Logger& logger, commands::Registry& registry);
extern "C" EMBER_EXPORT_SERVICE void destroy_login(Service* service);

} // login, ember