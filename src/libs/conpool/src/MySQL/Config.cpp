/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/MySQL/Config.h>
#include <boost/program_options.hpp>
#include <format>
#include <fstream>
#include <unordered_map>
#include <cstdint>

namespace ember::drivers {

using Options = std::unordered_map<std::string, std::string>;

namespace {

namespace po = boost::program_options;

po::variables_map parse_arguments(const std::string& config_path, const Options& opts) {

	// Config file options
	po::options_description config_opts("Configuration options");
	config_opts.add_options()
		(opts.at("username").c_str(), po::value<std::string>()->required())
		(opts.at("password").c_str(), po::value<std::string>()->default_value(""))
		(opts.at("database").c_str(), po::value<std::string>()->required())
		(opts.at("host").c_str(), po::value<std::string>()->required())
		(opts.at("port").c_str(), po::value<std::uint16_t>()->required());

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

MySQL init_db_driver(const std::string& config_path, std::string_view section) {
	const auto prefix = std::format("mysql.db.{}.", section);

	Options options;
	options["username"] = prefix + "username";
	options["password"] = prefix + "password";
	options["host"] = prefix + "host";
	options["port"] = prefix + "port";
	options["database"] = prefix + "database";

	auto args = parse_arguments(config_path, options);
	
	const auto& user = args[options["username"]].as<std::string>();
	const auto& pass = args[options["password"]].as<std::string>();
	const auto& host = args[options["host"]].as<std::string>();
	const auto& port = args[options["port"]].as<std::uint16_t>();
	const auto& db = args[options["database"]].as<std::string>();
	return {user, pass, host, port, db};
}

} // drivers, ember