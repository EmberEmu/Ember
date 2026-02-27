/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/pcp/Client.h>
#include <ports/upnp/SSDP.h>
#include <ports/upnp/IGDevice.h>
#include <ports/Utility.h>
#include <shared/utility/polyfill/print>
#include <boost/asio/ip/address.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <cstdint>
#include <cstdlib>

namespace opts = boost::program_options;

using namespace ember;

void launch(const opts::variables_map& args);
opts::variables_map parse_arguments(int argc, const char* argv[]);
void print_error(const ports::Error& error);
void use_upnp(const opts::variables_map& args);
void use_natpmp(const opts::variables_map& args);

int main(int argc, const char* argv[]) try {
	const auto args = parse_arguments(argc, argv);
	launch(args);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

void launch(const opts::variables_map& args) {
	if(args.contains("upnp")) {
		use_upnp(args);
	} else {
		use_natpmp(args);
	}
}

void print_error(const ports::Error& error) {
	std::println("Mapping error: {} ({})", error.code, std::to_underlying(error.code));

	if(error.code == ports::ErrorCode::pcp_code) {
		std::println("PCP code: {}", error.pcp_code);
	} else if(error.code == ports::ErrorCode::natpmp_code) {
		std::println("NAT-PMP code: {}", error.natpmp_code);
	}
}

void use_natpmp(const opts::variables_map& args) {
	const auto internal = args["internal"].as<std::uint16_t>();
	const auto external = args["external"].as<std::uint16_t>();
	const auto& interface = args["interface"].as<std::string>();
	const auto& gateway = args["gateway"].as<std::string>();
	const auto protocol = args["protocol"].as<ports::Protocol>();
	const auto deletion = args.contains("delete");
	
	const ports::MapRequest request {
		.protocol = protocol,
		.internal_port = internal,
		.external_port = external,
		.lifetime = deletion? 0u : 7200u
	};

	boost::asio::io_context ctx;

	ports::Client client(interface, gateway, ctx);

	// Create an Asio worker with a single thread
	auto worker = std::jthread(
		static_cast<size_t(boost::asio::io_context::*)()>(&boost::asio::io_context::run), &ctx
	);

	std::future<ports::Result> future;

	if(deletion) {
		future = client.delete_mapping(internal, protocol);
	} else {
		future = client.add_mapping(request, true);
	}

	auto result = future.get();

	if(result) {
		std::println("Successful {}: {} -> {} for {} seconds",
		             deletion? "deletion" : "mapping",
		             result->external_port,
		             result->internal_port,
		             result->lifetime);
	} else {
		std::println("Error: could not map port");
		print_error(result.error());
	}

	auto xfuture = client.external_address();
	const auto xresult = xfuture.get();

	if(xresult) {
		const auto v6 = boost::asio::ip::address_v6(xresult->external_ip);
		std::print("External address: {}", v6.to_string());
	} else {
		std::println("Error: could not retrieve external address");
		print_error(xresult.error());
	}

	ctx.stop();
}

void use_upnp(const opts::variables_map& args) {
	const auto& interface = args["interface"].as<std::string>();
	const auto protocol = args["protocol"].as<ports::Protocol>();
	const auto internal = args["internal"].as<std::uint16_t>();
	const auto external = args["external"].as<std::uint16_t>();
	const auto deletion = args.contains("delete");

	boost::asio::io_context ctx;
	ports::upnp::SSDP ssdp(interface, ctx);

	ssdp.locate_gateways([&](ports::upnp::LocateResult result) {
		if(!result) {
			std::cerr << result.error() << '\n';
			return true;
		}

		ports::upnp::Mapping map {
			.external = external,
			.internal = internal,
			.ttl = 0,
			.protocol = protocol
		};

		auto callback = [&, map](ports::upnp::ErrorCode ec) {
			if(!ec) {
				std::println("Port {} {} mapping successfully using UPnP",
				             map.external, deletion? "delete" : "add");
			} else {
				std::println("Port {} {} failed using UPnP, error {}",
				             map.external, deletion? "delete" : "add",
				             ec.value());
			}
		};
		
		if(deletion) {
			result->device->delete_port_mapping(map, callback);
		} else {
			result->device->add_port_mapping(map, callback);
		}

		return false;
	});

	ctx.run();
}

void protocol_validate(const ports::Protocol& protocol) {
	if(protocol == ports::Protocol::all) {
		throw std::invalid_argument(R"("all" is not a valid protocol option for this tool)");
	}
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	opts::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("upnp,u", "Use UPnP rather than NAT-PMP/PCP")
		("internal,i", opts::value<std::uint16_t>()->default_value(8085), "Internal port")
		("external,x", opts::value<std::uint16_t>()->default_value(8085), "External port")
		("interface,f", opts::value<std::string>()->default_value("0.0.0.0"), "Interface to bind to")
		("gateway,g", opts::value<std::string>()->default_value(""), "Gateway address")
		("delete,d", "Delete mapping")
		("protocol,p", opts::value<ports::Protocol>()->default_value(ports::Protocol::udp)->notifier(protocol_validate), "Protocol (udp, tcp)");

	opts::variables_map options;
	opts::store(opts::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	opts::notify(options);

	return options;
}