/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/MPQ.h>
#include <mpq/FileSink.h>
#include <iostream>

using namespace ember;

int main() try {
	std::filesystem::path path("test.MPQ");
	mpq::LocateResult result = mpq::locate_archive(path);

	if(!result) {
		std::cerr << "Not found, error " << result.error();
		return EXIT_FAILURE;
	}

	std::unique_ptr<mpq::Archive> archive { mpq::open_archive(path, *result)};

	if(!archive) {
		std::cerr << "No archive";
		return EXIT_FAILURE;
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
	return EXIT_SUCCESS;
} catch(std::exception& e) {
	std::cerr << e.what();
}