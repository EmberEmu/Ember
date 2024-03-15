/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/Archive.h>
#include <span>
#include <cstddef>

namespace ember::mpq {

class MemoryArchive : public Archive {
protected:
	std::span<std::byte> buffer_;
	std::span<BlockTableEntry> fetch_block_table() const;
	std::span<HashTableEntry> fetch_hash_table() const;

public:
	MemoryArchive(std::span<std::byte> buffer);

	int version() const override;
	std::size_t size() const override;
	Backing backing() const override { return Backing::MEMORY; }
	std::span<const BlockTableEntry> block_table() const override;
	std::span<const HashTableEntry> hash_table() const override;
	std::size_t file_lookup(std::string_view name, const std::uint16_t locale,
	                        const std::uint16_t platform) const override;
};


} // mpq, ember