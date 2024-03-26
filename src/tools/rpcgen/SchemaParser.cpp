/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SchemaParser.h"
#include <flatbuffers/reflection.h>
#include <flatbuffers/reflection_generated.h>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace ember {

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
		throw std::runtime_error("Unable to open binary schema (bfbs)");
	}

	const auto size = std::filesystem::file_size(path);
	buffer.resize(static_cast<std::size_t>(size));

	if(!file.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
		throw std::runtime_error("Unable to read binary schema (bfbs)");
	}

	return buffer;
}

void SchemaParser::generate(const std::filesystem::path& path) {
	const auto buffer = load_file(path);
	verify(buffer);
}

void SchemaParser::generate(std::span<const std::uint8_t> buffer) {
	verify(buffer);

	auto& s = *reflection::GetSchema(buffer.data());
	auto root_table = s.root_table();
	std::cout << root_table->name()->c_str();
}

} // ember