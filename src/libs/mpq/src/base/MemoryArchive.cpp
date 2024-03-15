/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/MemoryArchive.h>
#include <mpq/Crypt.h>
#include <mpq/Structures.h>
#include <boost/endian/conversion.hpp>
#include <bit>

#include <iostream> // todo

namespace ember::mpq {

MemoryArchive::MemoryArchive(std::span<std::byte> buffer) : buffer_(buffer) {
	auto btable = fetch_block_table();
	auto htable = fetch_hash_table();
	decrypt_block(std::as_writable_bytes(btable), MPQ_KEY_BLOCK_TABLE);
	decrypt_block(std::as_writable_bytes(htable), MPQ_KEY_HASH_TABLE);

	//auto listfile = buffer_.data() + 0x9A73A;
	//decrypt_block(listfile, KEY);
}

int MemoryArchive::version() const { 
	auto header = std::bit_cast<const v0::Header*, const std::byte*>(buffer_.data());
	return boost::endian::little_to_native(header->format_version);
}

std::size_t MemoryArchive::size() const {
	auto header = std::bit_cast<const v0::Header*, const std::byte*>(buffer_.data());
	return boost::endian::little_to_native(header->archive_size);
}

std::span<HashTableEntry> MemoryArchive::fetch_hash_table() const {
	auto header = std::bit_cast<const v0::Header*>(buffer_.data());
	auto entry = std::bit_cast<HashTableEntry*>(
		buffer_.data() + header->hash_table_offset
	);

	return { entry, header->hash_table_size };
}

std::span<BlockTableEntry> MemoryArchive::fetch_block_table() const {
	auto header = std::bit_cast<const v0::Header*>(buffer_.data());
	auto entry = std::bit_cast<BlockTableEntry*>(
		buffer_.data() + header->block_table_offset
	);

	return { entry, header->block_table_size };
}

std::size_t MemoryArchive::file_lookup(std::string_view name, const std::uint16_t locale,
                                       const std::uint16_t platform) const {
	const auto table = hash_table();
	auto index = hash_string(name, MPQ_HASH_TABLE_INDEX);
	index = index % (table.empty()? 0 : table.size());
	
	if(table[index].block_index == MPQ_HASH_ENTRY_EMPTY) {
		return npos;
	}

	const auto name_key_a = hash_string(name, MPQ_HASH_NAME_A);
	const auto name_key_b = hash_string(name, MPQ_HASH_NAME_B);
	const auto end = (index - 1) > table.size()? table.size() : index - 1;

	for(auto i = index; i != end; (++i) %= table.size()) {
		if(table[i].block_index == MPQ_HASH_ENTRY_EMPTY) {
			return npos;
		}

		if(table[i].name_1 == name_key_a  || table[i].name_2 == name_key_b
		   || table[i].locale == locale || table[i].platform == platform) {
			return i;
		}
	}

	return npos;
}

std::span<const BlockTableEntry> MemoryArchive::block_table() const {
	return fetch_block_table();
}

std::span<const HashTableEntry> MemoryArchive::hash_table() const {
	return fetch_hash_table();
}

} // mpq, ember