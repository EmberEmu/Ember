/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/SharedDefs.h>
#include <stdexcept>
#include <string>

namespace ember::mpq {

class unknown_format final : public std::runtime_error {
public:
	unknown_format() : std::runtime_error("unknown decompression method") { }
	unknown_format(const std::string& msg) : std::runtime_error(msg) { };
};

struct DecompressionError {
	bool unknown;
	Compression compression;
	int error;
};

} // mpq, ember