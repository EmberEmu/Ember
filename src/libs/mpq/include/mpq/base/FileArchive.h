/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/Archive.h>
#include <filesystem>

namespace ember::mpq {

class FileArchive : public Archive {
public:
	FileArchive(std::filesystem::path path, std::uintptr_t offset) {}

	int version() const override { return 0; }
	std::size_t size() const override { return 0; }
	Backing backing() const override { return Backing::FILE; }
	std::span<const BlockTableEntry> block_table() const override {
		return {};
	}
	std::span<const HashTableEntry> hash_table() const {
		return {};
	}

	std::size_t file_lookup(std::string_view name, const std::uint16_t locale,
	                        const std::uint16_t platform) const {
		return npos;
	}
};


} // mpq, ember