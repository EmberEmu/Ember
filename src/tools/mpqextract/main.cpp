/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/MPQ.h>
#include <mpq/Crypt.h>
#include <mpq/FileSink.h>
#include <cstdint>
#include <filesystem>
#include <iostream>

using namespace ember;

int main() try {
	std::filesystem::path path("test.MPQ");
	mpq::LocateResult result = mpq::locate_archive(path);

	if(!result) {
		std::cout << "Not found, error " << std::to_underlying(result.error());
		return -1;
	}

	std::unique_ptr<mpq::Archive> archive { mpq::open_archive(path, *result, true)};

	if(!archive) {
		std::cout << "No archive\n";
		return -1;
	}

	std::unique_ptr<mpq::v0::MappedArchive> archive_v0 {
		dynamic_cast<mpq::v0::MappedArchive*>(archive.release()) 
	};

	std::cout << "Type is: " << std::to_underlying(archive_v0->backing()) << '\n';

	auto header = archive_v0->header();
	std::cout << "Ver: " << std::hex << header->format_version << '\n';
	std::cout << "Size: " << header->archive_size << '\n';
	std::cout << "Block size: " << header->block_size_shift << '\n';
	std::cout << "BT offset: " << header->block_table_offset << '\n';
	std::cout << "BT size: " << header->block_table_size << '\n';
	std::cout << "HT offset: " << header->hash_table_offset << '\n';
	std::cout << "HT size: " << header->hash_table_size << '\n';
	const auto listed_files = archive_v0->files();

	for(auto& file : listed_files) {
		std::cout << file << '\n';
	}

	std::cout << "Files: " << listed_files.size();
} catch(std::exception& e) {
	std::cerr << e.what();
}