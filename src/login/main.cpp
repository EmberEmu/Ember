/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Logging.h>
#include <logger/ConsoleSink.h>
#include <logger/FileSink.h>
#include <logger/SyslogSink.h>
#include <logger/Utility.h>
#include <shared/pool/ConnectionPool.h>
#include <shared/Banner.h>
#include <shared/Version.h>
#include <botan/init.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <memory>
#include <fstream>
#include <string>

namespace po = boost::program_options;
namespace el = ember::log;

std::unique_ptr<ember::log::Logger> init_logging(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);
void launch(const po::variables_map& args, el::Logger* logger);

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	ember::print_banner("Login Daemon");
	const po::variables_map args = parse_arguments(argc, argv);

	auto logger = init_logging(args);
	el::set_global_logger(logger.get());
	LOG_INFO(logger) << "Initialised logger successfully" << LOG_SYNC;

	launch(args, logger.get());
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void launch(const po::variables_map& args, el::Logger* logger) try {
	Botan::LibraryInitializer init("thread_safe");
	LOG_INFO(logger) << "Initialised Botan successfully" << LOG_SYNC;

	unsigned int concurrency = std::thread::hardware_concurrency();

	if(!concurrency) {
		concurrency = 2;
	}

	boost::asio::io_service service;
	boost::asio::signal_set signals(service, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&boost::asio::io_service::stop, &service));

	std::vector<std::thread> workers;

	for(unsigned int i = 0; i < concurrency; ++i) {
		workers.emplace_back(std::bind(static_cast<size_t(boost::asio::io_service::*)()>
			(&boost::asio::io_service::run), &service)); 
	}

	LOG_INFO(logger) << "Launched " << concurrency << " login workers" << LOG_SYNC;

	for(auto& w : workers) {
		w.join();
	}

	LOG_INFO(logger) << "Login daemon shutting down..." << LOG_SYNC;
} catch(std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	//Command-line options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("config,c", po::value<std::string>()->default_value("login.conf"),
			"Path to the configuration file");

	po::positional_options_description pos; 
	pos.add("config", 1);

	//Config file options
	po::options_description config_opts("Login configuration options");
	config_opts.add_options()
		("networking.client_listen_ip,", po::value<std::string>()->default_value("0.0.0.0"))
		("networking.client_listen_port", po::value<unsigned int>()->default_value(3724))
		("console_log.verbosity,", po::value<std::string>()->required())
		("remote_log.verbosity,", po::value<std::string>()->required())
		("remote_log.service_name,", po::value<std::string>()->required())
		("remote_log.host,", po::value<std::string>()->required())
		("remote_log.port,", po::value<unsigned int>()->required())
		("file_log.verbosity,", po::value<std::string>()->required())
		("file_log.path,", po::value<std::string>()->default_value("login.log"))
		("file_log.timestamp_format,", po::value<std::string>())
		("file_log.mode,", po::value<std::string>()->required())
		("file_log.size_rotate,", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate,", po::bool_switch()->required())
		("file_log.log_timestamp,", po::bool_switch()->required())
		("file_log.log_severity,", po::bool_switch()->required())
		("database.username", po::value<std::string>()->required())
		("database.password", po::value<std::string>())
		("database.database", po::value<std::string>()->required())
		("database.host", po::value<std::string>()->required())
		("database.port", po::value<unsigned int>()->required());

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	std::string config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		std::string message("Unable to open configuration file: " + config_path);
		throw std::invalid_argument(message);
	}

	po::store(po::parse_config_file(ifs, config_opts), options);
	po::notify(options);

	return std::move(options);
}

std::unique_ptr<el::Sink> init_remote_sink(const po::variables_map& args, el::SEVERITY severity) {
	std::string host = args["remote_log.host"].as<std::string>();
	std::string service = args["remote_log.service_name"].as<std::string>();
	unsigned int port = args["remote_log.port"].as<unsigned int>();
	auto facility = el::SyslogSink::FACILITY::LOCAL_USE_0;
	return std::make_unique<el::SyslogSink>(severity, host, port, facility, service);
}

std::unique_ptr<el::Sink> init_file_sink(const po::variables_map& args, el::SEVERITY severity) {
	std::string mode_str = args["file_log.mode"].as<std::string>();
	std::string path = args["file_log.path"].as<std::string>();

	if(mode_str != "append" && mode_str != "truncate") {
		throw std::runtime_error("Invalid file logging mode supplied");
	}

	el::FileSink::MODE mode = (mode_str == "append")? el::FileSink::MODE::APPEND :
	                                                  el::FileSink::MODE::TRUNCATE;

	auto file_sink = std::make_unique<el::FileSink>(severity, path, mode);
	file_sink->size_limit( args["file_log.size_rotate"].as<std::uint32_t>());
	file_sink->log_severity(args["file_log.log_timestamp"].as<bool>());
	file_sink->log_date(args["file_log.log_timestamp"].as<bool>());
	file_sink->time_format(args["file_log.timestamp_format"].as<std::string>());
	file_sink->midnight_rotate(args["file_log.midnight_rotate"].as<bool>());
	return std::move(file_sink);
}

std::unique_ptr<el::Sink> init_console_sink(const po::variables_map& args, el::SEVERITY severity) {
	return std::make_unique<el::ConsoleSink>(severity);
}

std::unique_ptr<ember::log::Logger> init_logging(const po::variables_map& args) {
	auto logger = std::make_unique<el::Logger>();
	el::SEVERITY severity;

	if((severity = el::severity_string(args["console_log.verbosity"].as<std::string>()))
		!= el::SEVERITY::DISABLED) {
		logger->add_sink(init_console_sink(args, severity));
	}

	if((severity = el::severity_string(args["file_log.verbosity"].as<std::string>()))
		!= el::SEVERITY::DISABLED) {
		logger->add_sink(init_file_sink(args, severity));
	}

	if((severity = el::severity_string(args["remote_log.verbosity"].as<std::string>()))
		!= el::SEVERITY::DISABLED) {
		logger->add_sink(init_remote_sink(args, severity));
	}

	return logger;
}