/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Client.h>
#include <stun/Utility.h>
#include <boost/asio/ip/address.hpp>
#include <boost/program_options.hpp>
#include <stun/Client.h>
#include <stun/Transport.h>
#include <stdexcept>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>

namespace po = boost::program_options;

using namespace ember;

void launch(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);
void log_cb(stun::Verbosity verbosity, stun::Error reason);
void print_error(std::string_view test, const stun::ErrorRet& error);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	launch(args);
} catch(const std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void launch(const po::variables_map& args) {
	const auto host = args["host"].as<std::string>();
	const auto port = args["port"].as<std::uint16_t>();
	const auto protocol = args["protocol"].as<std::string>();
	
	// todo, std::print when supported by all compilers
	std::cout << std::format("Using {}:{} ({}) as our STUN server\n", host, port, protocol);

	const auto proto = protocol == "tcp"? stun::Protocol::TCP : stun::Protocol::UDP;

	stun::Client client(host, port, proto);
	client.log_callback(log_cb, stun::Verbosity::STUN_LOG_TRIVIAL);
	auto result = client.external_address();
	const auto address = result.get();

	if(address) {
		const std::string& ip = stun::extract_ip_to_string(*address);

		// todo, std::print when supported by all compilers
		std::cout << std::format("STUN provider returned our address as {}:{}\n", ip, address->port);
	} else {
		print_error("STUN", address.error());
	}

	auto nat_res = client.nat_present();
	const auto nat_detected = nat_res.get();

	if(nat_detected) {
		std::cout << std::format("NAT detected: {}\n", *nat_detected? "Yes" : "No");
	} else {
		print_error("STUN", nat_detected.error());
	}

	auto map_res = client.mapping();
	const auto mapping = map_res.get();

	if(mapping) {
		std::cout << std::format("Mapping type: {}\n", stun::to_string(*mapping));
	} else {
		print_error("Mapping", mapping.error());
	}
	
	auto hairpin_res = client.hairpinning();
	const auto hairpinning = hairpin_res.get();

	if(hairpinning) {
		std::cout << std::format("Hairpin: {}\n", stun::to_string(*hairpinning));
	} else {
		print_error("Hairpin", hairpinning.error());
	}

	auto filter_res = client.filtering();
	const auto filtering = filter_res.get();

	if(filtering) {
		std::cout << std::format("Filtering: {}\n", stun::to_string(*filtering));
	} else {
		print_error("Filtering", filtering.error());
	}
}

void print_error(std::string_view test, const stun::ErrorRet& error) {
	std::cout << std::format("{} test failed: {} ({})\n", test,
		stun::to_string(error.reason),
		std::to_underlying(error.reason));

	if(error.ec.code) {
		std::cout << std::format("Server error code: {}, ({})\n",
			error.ec.code, error.ec.reason);
	}
}

void log_cb(const stun::Verbosity verbosity, const stun::Error reason) {
	std::string_view verbstr{};

	switch(verbosity) {
		case stun::Verbosity::STUN_LOG_TRIVIAL:
			verbstr = "[trivial]";
			break;
		case stun::Verbosity::STUN_LOG_DEBUG:
			verbstr = "[debug]";
			break;
		case stun::Verbosity::STUN_LOG_INFO:
			verbstr = "[info]";
			break;
		case stun::Verbosity::STUN_LOG_WARN:
			verbstr = "[warn]";
			break;
		case stun::Verbosity::STUN_LOG_ERROR:
			verbstr = "[error]";
			break;
		case stun::Verbosity::STUN_LOG_FATAL:
			verbstr = "[fatal]";
			break;
		default:
			verbstr = "[unknown]";
	}

	// todo, std::print when supported by all compilers
	std::cout << std::format("{} {} ({})\n",
		verbstr, stun::to_string(reason), std::to_underlying(reason));
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("host,h", po::value<std::string>()->default_value("stunserver.stunprotocol.org"), "Host")
		("port,p", po::value<std::uint16_t>()->default_value(3479), "Port")
		("protocol,c", po::value<std::string>()->default_value("tcp"), "Protocol (udp, tcp)");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	po::notify(options);

	return options;
}