#include <shared/Version.h>
#include <shared/Banner.h>
#include <botan/init.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;

void verify_port(int port);
po::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	ember::print_banner("Login Daemon");
	const po::variables_map arguments = parse_arguments(argc, argv);
	//Botan::LibraryInitializer init("thread_safe");
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	//Command-line only options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("config,c", po::value<std::string>()->default_value("login.conf"),
			"Path to the configuration file");

	//Config file only options
	po::options_description config_opts("Login configuration options");
	config_opts.add_options()
		("networking.ip,", po::value<std::string>()->default_value("0.0.0.0"))
		("networking.port", po::value<int>()->default_value(3724)->notifier(&verify_port))
		("database.username", po::value<std::string>()->required())
		("database.password", po::value<std::string>())
		("database.database", po::value<std::string>()->required())
		("database.host", po::value<std::string>()->required())
		("database.port", po::value<int>()->required()->notifier(&verify_port));

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);
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

void verify_port(int port) {
	if (port <= 0 || port > 65535) {
		throw std::invalid_argument("Specified port outside of valid range (> 0 &  <= 65535)!");
	}
}