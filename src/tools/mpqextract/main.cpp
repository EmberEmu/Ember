/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/MPQ.h>
#include <cstdint>
#include <filesystem>
#include <iostream>

using namespace ember;

int main() {
	std::filesystem::path path("test.MPQ");
	mpq::LocateResult result = mpq::locate_archive(path);

	if(result) {
		std::cout << "Found at " << *result;
	} else {
		std::cout << "Not found, error " << std::to_underlying(result.error());
	}
}