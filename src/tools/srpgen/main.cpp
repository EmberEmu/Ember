/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Generator.h>
#include <srp6/Util.h>
#include <boost/program_options.hpp>
#include <algorithm>
#include <exception>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <cctype>
#include <string>

namespace po = boost::program_options;

void launch(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);

int main(int argc, const char* argv[]) try {
	const po::variables_map args = parse_arguments(argc, argv);
	launch(args);
} catch(const std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void launch(const po::variables_map& args) {
	using namespace ember;

	auto username = args["username"].as<std::string>();
	auto password = args["password"].as<std::string>();

	std::transform(username.begin(), username.end(), username.begin(), ::toupper);
	std::transform(password.begin(), password.end(), password.begin(), ::toupper);

	auto gen = srp6::Generator(srp6::Generator::Group::_256_BIT);
	auto salt = srp6::generate_salt<32>();
	auto verifier = srp6::generate_verifier(username, password, gen, salt, srp6::Compliance::GAME);

	std::cout << "Username: " << username << "\n";
	std::cout << "Verifier: " << "0x" << std::hex << verifier << "\n";
	std::cout << "Salt: ";

	for(auto byte : salt) {
		std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
	}

	if(args["sbin"].empty()) {
		return;
	}

	const auto& file = args["sbin"].as<std::string>();
	std::ofstream stream(file, std::ios::binary | std::ios::trunc);

	if(!stream) {
		throw std::runtime_error("\nCould not open salt output file, " + file);
	}

	for(auto byte : salt) {
		stream << byte;
	}

	if (!stream) {
		throw std::runtime_error("\nAn error occurred while attempting to write salt to file, " + file);
	}
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("username,u", po::value<std::string>()->required(), "Username")
		("password,p", po::value<std::string>()->required(), "Password")
		("sbin,s",     po::value<std::string>(), "Output salt into a binary file");

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help") || argc <= 1) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	po::notify(options);

	return options;
}