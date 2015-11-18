#include "TestService.h"
#include <spark/Spark.h>
#include <shared/Banner.h>
#include <shared/util/LogConfig.h>
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <cstdint>

namespace el = ember::log;
namespace po = boost::program_options;
namespace ba = boost::asio;

void launch(const po::variables_map& args, el::Logger* logger);
po::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);

	auto logger = ember::util::init_logging(args);
	el::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	launch(args, logger.get());
	LOG_INFO(logger) << "Bye" << LOG_SYNC;
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void test_func(ember::spark::Service* spark) {
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	for(;;) {
		spark->connect("::1", 6000);
		spark->connect("::1", 6000);
		spark->connect("::1", 6000);
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

void launch(const po::variables_map& args, el::Logger* logger) try {
	ba::io_service service;
	ember::spark::Service spark("Test Program", service, "0.0.0.0", 6001, logger, el::Filter(1));
	ember::TestService test(spark, logger);
	spark.connect("::1", 6000);
	service.run();
	
} catch(std::exception& e) {
	MessageBox(NULL, e.what(), 0, 0);
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	//Command-line options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("database.config_path,d", po::value<std::string>(),
		 "Path to the database configuration file")
		("config,c", po::value<std::string>()->default_value("test.conf"),
		 "Path to the configuration file");

	po::positional_options_description pos;
	pos.add("config", 1);

	//Config file options
	po::options_description config_opts("Configuration options");
	config_opts.add_options()
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("console_log.verbosity", po::value<std::string>()->required())
		("console_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("remote_log.verbosity", po::value<std::string>()->required())
		("remote_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", po::value<std::string>()->required())
		("remote_log.host", po::value<std::string>()->required())
		("remote_log.port", po::value<std::uint16_t>()->required())
		("file_log.verbosity", po::value<std::string>()->required())
		("file_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("file_log.path", po::value<std::string>()->default_value("test.log"))
		("file_log.timestamp_format", po::value<std::string>())
		("file_log.mode", po::value<std::string>()->required())
		("file_log.size_rotate", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", po::bool_switch()->required())
		("file_log.log_timestamp", po::bool_switch()->required())
		("file_log.log_severity", po::bool_switch()->required());

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

	return options;
}