/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/MPQ.h>
#include <mpq/Crypt.h>
#include <mpq/FileSink.h>
#include <mpq/Utility.h>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>

using namespace ember;

int main() try {
	std::filesystem::path path("test.MPQ");
	mpq::LocateResult result = mpq::locate_archive(path);

	if(!result) {
		std::cout << "Not found, error " << std::to_underlying(result.error());
		return -1;
	}

	std::unique_ptr<mpq::Archive> archive { mpq::open_archive(path, *result)};

	if(!archive) {
		std::cout << "No archive\n";
		return -1;
	}

	for(auto& f : archive->files()) {
		try {
			mpq::FileSink sink(f);
			archive->extract_file(f, sink);
		} catch(mpq::exception& e) {
			std::cerr << std::format("{} ({})", e.what(), f);
		}
	}

	std::cout << "Zug zug, work complete!";
} catch(std::exception& e) {
	std::cerr << e.what();
}