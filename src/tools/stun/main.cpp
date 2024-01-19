/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Client.h>
#include <boost/asio/ip/address.hpp>
#include <boost/program_options.hpp>
#include <format>
#include <iostream>
#include <utility>
#include <cstdint>

namespace po = boost::program_options;

using namespace ember;

void launch(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);
void log_cb(stun::Verbosity verbosity, stun::Error reason);

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
	
	auto proto = stun::Protocol::UDP;

	if (protocol == "tcp") {
		proto = stun::Protocol::TCP;
	} else if (protocol == "tls_tcp") {
		proto = stun::Protocol::TLS_TCP;
	} else if (protocol != "udp") {
		throw std::invalid_argument("Unknown protocol specified");
	}

	// todo, std::print when supported by all compilers
	std::cout << std::format("Using {}:{} ({}) as our STUN server\n", host, port, protocol);

	stun::Client client;
	client.log_callback(log_cb, stun::Verbosity::STUN_LOG_TRIVIAL);
	client.connect(host, port, proto);
	auto result = client.external_address();
	const auto address = result.get();

	if(address) {
		std::string addr_str;

		if(address->family == stun::AddressFamily::IPV4) {
			addr_str = boost::asio::ip::address_v4(address->ipv4).to_string();
		} else {
			boost::asio::ip::address_v6::bytes_type bytes;
			std::copy(address->ipv6.begin(), address->ipv6.end(), bytes.data());
			addr_str = boost::asio::ip::address_v6(bytes).to_string();
		}

		// todo, std::print when supported by all compilers
		std::cout << std::format("STUN provider returned our address as {}:{}", addr_str, address->port);
	} else {
		std::cout << std::format("STUN request failed: {} ({})",
			stun::to_string(address.error()), std::to_underlying(address.error()));
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
	std::cout << std::format("{} {} ({})\n", verbstr, stun::to_string(reason), std::to_underlying(reason));
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("host,h", po::value<std::string>()->default_value("stun.l.google.com"), "Host")
		("port,p", po::value<std::uint16_t>()->default_value(19302), "Port")
		("protocol,c", po::value<std::string>()->default_value("udp"), "Protocol (udp, tcp, tls_tcp)");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	po::notify(options);

	return options;
}