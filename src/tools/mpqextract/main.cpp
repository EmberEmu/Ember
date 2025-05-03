/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/MPQ.h>
#include <mpq/FileSink.h>
#include <mpq/BufferedFileSink.h>
#include <shared/utility/polyfill/print>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace ember;

int main(int argc, const char* argv[]) try {
	if(argc < 2) {
		std::print(
			"Usage: mpqextract <input.mpq> [--v] [ecma regex]\n"
			"Add --v for verbose printing\n\n"
			"Only files matching the optional regex will be extracted\n\n"
			R"a(Example: mpqextract.exe test.mpq "([a-zA-Z0-9\s_\\.\-\(\):])+(.jpg|.gif)")a"
		);

		return EXIT_FAILURE;
	}

	std::filesystem::path path(argv[1]);
	mpq::LocateResult result = mpq::locate_archive(path);

	if(!result) {
		std::print("Not found, error {}", result.error());
		return EXIT_FAILURE;
	}

	std::unique_ptr<mpq::Archive> archive = mpq::open_archive(path, *result);

	if(!archive) {
		std::print("No archive");
		return EXIT_FAILURE;
	}

	int pattern_index = 0;
	bool verbose = false;

	if(argc >= 3) {
		for(int i = 2; i < argc; ++i) {
			if(std::strcmp(argv[i], "--v") == 0) {
				verbose = true;
			} else {
				pattern_index = i;
			}
		}
	}

	const std::string pattern = pattern_index? argv[pattern_index] : "";
	const std::regex regex(pattern, std::regex::ECMAScript);

	for(auto& f : archive->files()) try {
		if(!pattern.empty()) {
			if(!std::regex_match(f, regex)) {
				continue;
			}
		}
		
		const auto index = archive->file_lookup(f, 0);

		if(index == mpq::npos) {
			std::println("Skipping: {} found in listfile but not indexed", f);
			continue;
		}

		const auto& entry = archive->file_entry(index);
		mpq::BufferedFileSink sink(f, entry.uncompressed_size);

		if(verbose) {
			std::println("Extracting {} ({} bytes)... ", f, entry.uncompressed_size);
		}

		archive->extract_file(f, sink);
	} catch(const mpq::exception& e) {
		std::println("Extraction error: {} ({})", e.what(), f);
	}

	std::print("Zug zug, work complete!");
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	std::print("Fatal error: {}", e.what());
	return EXIT_FAILURE;
}