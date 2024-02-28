/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <portmap/natpmp/Client.h>
#include <boost/asio/ip/address.hpp>
#include <boost/program_options.hpp>
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
void print_error(const portmap::natpmp::Error& error);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	launch(args);
} catch(const std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void launch(const po::variables_map& args) {
	const auto internal = args["internal"].as<std::uint16_t>();
	const auto external = args["external"].as<std::uint16_t>();
	const auto interface = args["interface"].as<std::string>();
	const auto gateway = args["gateway"].as<std::string>();
	const auto protocol = args["protocol"].as<std::string>();
	const auto deletion = args["delete"].as<bool>();
	
	auto proto = portmap::natpmp::Protocol::MAP_TCP;

	if(protocol == "udp") {
		proto = portmap::natpmp::Protocol::MAP_UDP;
	}

	const portmap::natpmp::RequestMapping request {
		.opcode = proto,
		.internal_port = internal,
		.external_port = external,
		.lifetime = deletion? 0u : 7200u
	};

	portmap::natpmp::Client client(interface, gateway);
	std::future<portmap::natpmp::Client::MapResult> future;

	if(deletion) {
		future = client.delete_mapping(internal, proto);
	} else {
		future = client.add_mapping(request);
	}

	auto result = future.get();

	if(result) {
		std::cout << std::format("Successful {}: {} -> {} for {} seconds\n",
		                         deletion? "deletion" : "mapping",
		                         result->external_port,
		                         result->internal_port,
		                         result->lifetime);
	} else {
		std::cout << "Error: could not map port" << std::endl;
		print_error(result.error());
	}

	auto xfuture = client.external_address();
	const auto xresult = xfuture.get();

	if(xresult) {
		const auto v6 = boost::asio::ip::address_v6(*xresult);
		std::cout << std::format("External address: {}", v6.to_string());
	} else {
		std::cout << "Error: could not retrieve external address" << std::endl;
		print_error(result.error());
	}
}

void print_error(const portmap::natpmp::Error& error) {
	std::cout << std::format("Mapping error: {} ({})\n",
		error.type, std::to_underlying(error.type));

	if(error.type == portmap::natpmp::ErrorType::PCP_CODE) {
		std::cout << std::format("PCP code: {}\n", error.pcp_code);
	} else if(error.type == portmap::natpmp::ErrorType::NATPMP_CODE) {
		std::cout << std::format("NAT-PMP code: {}\n", error.natpmp_code);
	}
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("internal,i", po::value<std::uint16_t>()->default_value(8085), "Internal port")
		("external,x", po::value<std::uint16_t>()->default_value(8085), "External port")
		("interface,f", po::value<std::string>()->default_value("0.0.0.0"), "Interface to bind to")
		("gateway,g", po::value<std::string>()->default_value(""), "Gateway address")
		("delete,d", po::value<bool>()->default_value(false), "Delete mapping")
		("protocol,p", po::value<std::string>()->default_value("udp"), "Protocol (udp, tcp)");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	po::notify(options);

	return options;
}