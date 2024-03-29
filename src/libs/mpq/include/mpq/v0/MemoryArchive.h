/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/MemoryArchive.h>
#include <mpq/Structures.h>

namespace ember::mpq::v0 {

class MemoryArchive : public mpq::MemoryArchive {
	std::span<BlockTableEntry> fetch_block_table() const;
	std::span<HashTableEntry> fetch_hash_table() const;

public:
	MemoryArchive(std::span<std::byte> buffer);
	const Header* header() const;
	std::size_t size() const override;
	void extract_file(const std::filesystem::path& path, ExtractionSink& store) override;
};

} // v0, mpq, ember