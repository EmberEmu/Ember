/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <srp6/Generator.h>
#include <srp6/Utility.h>
#include "nlohmann/json.hpp"
#include <boost/program_options.hpp>
#include <algorithm>
#include <exception>
#include <format>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <cctype>
#include <cstdlib>

using namespace nlohmann;
namespace opts = boost::program_options;

void launch(const opts::variables_map& args);
opts::variables_map parse_arguments(int argc, const char* argv[]);
void plaintext_output(const std::string_view username, const Botan::BigInt& verifier, std::span<std::uint8_t> salt);
void json_output(const std::string_view username, const Botan::BigInt& verifier, std::span<std::uint8_t> salt);

int main(int argc, const char* argv[]) try {
	const auto args = parse_arguments(argc, argv);
	launch(args);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

void launch(const opts::variables_map& args) {
	using namespace ember;

	auto username = args["username"].as<std::string>();
	auto password = args["password"].as<std::string>();

	const auto upper = [](const unsigned char c) {
		return std::toupper(c);
	};

	std::ranges::transform(username, username.begin(), upper);
	std::ranges::transform(password, password.begin(), upper);

	auto gen = srp6::Generator(srp6::Generator::Group::g_256_bit);
	std::array<std::uint8_t, 32> salt;
	srp6::generate_salt(salt);
	auto verifier = srp6::generate_verifier(username, password, gen, salt, srp6::Compliance::game);

	if(args["json"].as<bool>()) {
		json_output(username, verifier, salt);
	} else {
		plaintext_output(username, verifier, salt);
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

	if(!stream) {
		throw std::runtime_error("\nAn error occurred while attempting to write salt to file, " + file);
	}
}

void plaintext_output(const std::string_view username, const Botan::BigInt& verifier, std::span<std::uint8_t> salt) {
	std::cout << "Username: " << username << "\n";
	std::cout << "Verifier: " << verifier.to_hex_string() << "\n";
	std::cout << "Salt: ";

	for(auto byte : salt) {
		std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
	}
}

void json_output(const std::string_view username, const Botan::BigInt& verifier, std::span<std::uint8_t> salt) {
	const auto vstr = std::format("0x{}", verifier.to_hex_string());
	const auto saltdec = Botan::BigInt::from_bytes(salt);
	const auto sstr = std::format("0x{}",  saltdec.to_hex_string());

	json data;
	data["username"] = username;
	data["verifier"] = vstr;
	data["salt"] = sstr;
	std::cout << data.dump(4);
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	opts::options_description cmdline_opts("Options");
	cmdline_opts.add_options()
		("help,h",     "Displays a list of available options")
		("username,u", opts::value<std::string>()->required(), "Username")
		("password,p", opts::value<std::string>()->required(), "Password")
		("sbin,s",     opts::value<std::string>(), "Output salt into a binary file")
		("json,j",     opts::bool_switch(), "Output parameters as JSON");

	opts::variables_map options;
	opts::store(opts::command_line_parser(argc, argv).options(cmdline_opts).run(), options);

	if(options.count("help") || argc <= 1) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	opts::notify(options);

	return options;
}