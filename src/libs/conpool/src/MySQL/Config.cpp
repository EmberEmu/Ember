/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/MySQL/Config.h>
#include <boost/program_options.hpp>
#include <string>
#include <fstream>
#include <cstdint>

namespace ember::drivers {

namespace {

namespace po = boost::program_options;

po::variables_map parse_arguments(const std::string& config_path) {
	//Config file options
	po::options_description config_opts("Configuration options");
	config_opts.add_options()
		("mysql.username", po::value<std::string>()->required())
		("mysql.password", po::value<std::string>()->default_value(""))
		("mysql.database", po::value<std::string>()->required())
		("mysql.host", po::value<std::string>()->required())
		("mysql.port", po::value<std::uint16_t>()->required());

	po::variables_map options;
	std::ifstream ifs(config_path);

	if(!ifs) {
		std::string message("Unable to open configuration file: " + config_path);
		throw std::invalid_argument(message);
	}

	po::store(po::parse_config_file(ifs, config_opts), options);
	po::notify(options);

	return options;
}

} // unnamed

MySQL init_db_driver(const std::string& config_path) {
	auto args = std::move(parse_arguments(config_path));
	const auto& user = args["mysql.username"].as<std::string>();
	const auto& pass = args["mysql.password"].as<std::string>();
	const auto& host = args["mysql.host"].as<std::string>();
	const auto& port = args["mysql.port"].as<std::uint16_t>();
	const auto& db = args["mysql.database"].as<std::string>();
	return {user, pass, host, port, db};
}

} // drivers, ember