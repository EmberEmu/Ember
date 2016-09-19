/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <string>
#include <sstream>
#include <cstdint>
#include <string>

namespace ember {

struct FileMeta {
	std::string path;
	std::string name;
	std::array<char, 16> md5;
	std::uint64_t size;
};

struct PatchMeta {
	std::uint32_t id;
	std::uint16_t build_from;
	std::uint16_t build_to;
	std::uint32_t arch_id;
	std::uint32_t locale_id;
	std::uint32_t os_id;
	std::string arch;
	std::string locale;
	std::string os;
	bool mpq;
	bool rollup;
	FileMeta file_meta;
};

} // ember