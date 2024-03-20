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

namespace ember::mpq::v1 {

class MemoryArchive : public mpq::MemoryArchive {
	std::span<BlockTableEntry> fetch_block_table() const;
	std::span<HashTableEntry> fetch_hash_table() const;
	v1::Header* header_;
	std::span<std::uint16_t> bt_hi_pos_;

	std::span<std::uint16_t> fetch_btable_hi_pos() const;
	std::uint64_t high_mask(std::uint16_t value) const;

public:
	MemoryArchive(std::span<std::byte> buffer);
	const Header* header() const;
	std::size_t size() const override;
	void extract_file(const std::filesystem::path& path, ExtractionSink& store) override;
};

} // v1, mpq, ember