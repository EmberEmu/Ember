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
#include <span>
#include <string>
#include <vector>
#include <cstddef>

namespace ember::mpq {

class ExtractionSink;

class MemoryArchive : public Archive {
protected:
	std::span<std::byte> buffer_;
	std::span<BlockTableEntry> block_table_;
	std::span<HashTableEntry> hash_table_;
	const v0::Header* header_;
	std::vector<std::string> files_;
	
	BlockTableEntry& file_entry(std::size_t index);
	void load_listfile(std::uint64_t fpos_hi);
	void extract_compressed(BlockTableEntry& entry, std::uint32_t key,
	                        std::uint64_t fpos_hi, ExtractionSink& store);
	void extract_uncompressed(BlockTableEntry& entry, std::uint32_t key,
	                          std::uint64_t fpos_hi, ExtractionSink& store);
	void extract_file_ext(const std::filesystem::path& path, ExtractionSink& store,
	                      std::uint64_t fpos_hi);

public:
	MemoryArchive(std::span<std::byte> buffer);

	int version() const override;
	Backing backing() const override { return Backing::MEMORY; }
	std::span<const BlockTableEntry> block_table() const override;
	std::span<const HashTableEntry> hash_table() const override;
	std::size_t file_lookup(std::string_view name, const std::uint16_t locale) const override;
	std::span<std::uint32_t> file_sectors(const BlockTableEntry& entry);
	std::span<const std::string> files() const override;
	void files(std::span<std::string_view> files) override;
};


} // mpq, ember