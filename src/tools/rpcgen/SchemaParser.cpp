/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SchemaParser.h"
#include "inja/inja.hpp"
#include <flatbuffers/reflection.h>
#include <chrono>
#include <format>
#include <fstream>
#include <stdexcept>
#include <iostream>

using namespace nlohmann;

namespace ember {

SchemaParser::SchemaParser(std::filesystem::path templates_dir, std::filesystem::path output_dir)
	: tpl_path_(std::move(templates_dir)),
      out_path_(std::move(output_dir)) {
}

void SchemaParser::verify(std::span<const std::uint8_t> buffer) {
	flatbuffers::Verifier verifier(buffer.data(), buffer.size());

	if(!reflection::VerifySchemaBuffer(verifier)) {
		throw std::runtime_error("Schema verification failed!");
	}
}

std::vector<std::uint8_t> SchemaParser::load_file(const std::filesystem::path& path) {
	std::vector<std::uint8_t> buffer;
	std::ifstream file(path, std::ios::in | std::ios::binary);

	if(!file.is_open()) {
		throw std::runtime_error(std::format("Unable to open, {}", path.string()));
	}

	const auto size = std::filesystem::file_size(path);
	buffer.resize(static_cast<std::size_t>(size));

	if(!file.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
		throw std::runtime_error(std::format("Unable to read, {}", path.string()));
	}

	return buffer;
}

void SchemaParser::generate(const std::filesystem::path& path) {
	const auto buffer = load_file(path);
	generate(buffer);
}

void SchemaParser::generate(std::span<const std::uint8_t> buffer) {
	verify(buffer);

	const auto& schema = reflection::GetSchema(buffer.data());
	const auto services = schema->services();

	for(const auto& service : *services) {
		process(service);
	}
}

std::string SchemaParser::remove_namespace(const std::string& name) {
	auto loc = name.find_last_of('.');

	if(loc == std::string::npos) {
		return name;
	}

	return name.substr(loc + 1);
}

void SchemaParser::process(const reflection::Service* service) {
	const auto time = std::chrono::system_clock::now();
	const std::chrono::year_month_day date = std::chrono::floor<std::chrono::days>(time);
	const auto bare_name = remove_namespace(service->name()->str());

	json data;
	data["fbs_name"] = bare_name;
	data["year"] = static_cast<int>(date.year());
	data["name"] = bare_name;
	data["handlers"] = {};

	for(auto call : *service->calls()) {
		data["handlers"].push_back(
			{
				{"call", call->name()->str() },
				{"name", snake_case(call->name()->str()) },
				{"request_ns", to_cpp_ns(call->request()->name()->str())},
				{"request", remove_namespace(call->request()->name()->str())},
				{"response_ns", to_cpp_ns(call->response()->name()->str())},
				{"response", remove_namespace(call->response()->name()->str())},
			}
		);
	}
	
	inja::Environment env;
	auto tpl = env.parse_template(tpl_path_.string() + "Service.h_");

	const auto path = std::format(
		"{}/{}ServiceStub.h", out_path_.string(), data["name"].get<std::string>()
	);

	env.write(tpl, data, path);
}

std::string SchemaParser::to_cpp_ns(const std::string& val) {
	std::string result = val;

	for(auto it = result.begin(); it != result.end();) {
		if(*it == '.') {
			*it = ':';
			result.insert(it, ':');
		}

		++it;
	}

	return result;
}

std::string SchemaParser::snake_case(const std::string& val) {
	std::string result;
	bool first = true;

	for(auto ch : val) {
		if(std::isupper(ch)) { 
			if(!first) {
				result = result + '_';
			}

			result += std::tolower(ch);
		} else { 
			result = result + ch; 
		}

		first = false;
	} 

	return result; 
}

} // ember