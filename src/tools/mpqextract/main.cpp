/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/MPQ.h>
#include <mpq/FileSink.h>
#include <mpq/BufferedFileSink.h>
#include <iostream>
#include <regex>
#include <cstdlib>

using namespace ember;

int main(int argc, char** argv) try {
	if(argc < 2) {
		std::cout << "Usage: mpqextract <input.mpq> [ecma regex]\n"
			<< "Only files matching the optional regex will be extracted\n\n"
			<< R"a(Example: mpqextract.exe test.mpq "([a-zA-Z0-9\s_\\.\-\(\):])+(.jpg|.gif)")a";

		return EXIT_FAILURE;
	}

	std::filesystem::path path(argv[1]);
	mpq::LocateResult result = mpq::locate_archive(path);

	if(!result) {
		std::cerr << "Not found, error " << result.error();
		return EXIT_FAILURE;
	}

	std::unique_ptr<mpq::Archive> archive = mpq::open_archive(path, *result);

	if(!archive) {
		std::cerr << "No archive";
		return EXIT_FAILURE;
	}

	const std::string pattern = argc >= 3? argv[2] : "";
	const std::regex regex(pattern, std::regex::ECMAScript);

	for(auto& f : archive->files()) try {
		if(!pattern.empty()) {
			if(!std::regex_match(f, regex)) {
				continue;
			}
		}
		
		auto index = archive->file_lookup(f, 0);

		if(index == mpq::npos) {
			continue;
		}

		const auto& entry = archive->file_entry(index);
		mpq::BufferedFileSink sink(f, entry.uncompressed_size);
		archive->extract_file(f, sink);
		sink.flush();
	} catch(mpq::exception& e) {
		std::cerr << std::format("{} ({})\n", e.what(), f);
	}

	std::cout << "Zug zug, work complete!";
	return EXIT_SUCCESS;
} catch(std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}