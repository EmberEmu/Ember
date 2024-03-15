/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/base/MemoryArchive.h>
#include <mpq/Crypt.h>
#include <mpq/Structures.h>
#include <boost/endian/conversion.hpp>
#include <boost/container/small_vector.hpp>
#include <bit>
#include <vector>
#include <zlib.h> // todo
#include <iostream> // todo
#include <fstream> // todo
#include <cmath>

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

std::span<std::uint32_t> MemoryArchive::file_sectors(BlockTableEntry& entry) {
	const auto header = std::bit_cast<const v0::Header*>(buffer_.data());
	const auto sector_size = BLOCK_SIZE << header->block_size_shift;
	auto count = (uint32_t)std::floor(entry.uncompressed_size / sector_size + 1); // todo

	if(entry.flags & Flag::MPQ_FILE_SECTOR_CRC) {
		++count;
	}

	auto sector_begin = std::bit_cast<std::uint32_t*>(buffer_.data() + entry.file_position);
	std::span<std::uint32_t> sectors { sector_begin, count + 1 };

	if(entry.flags & Flag::MPQ_FILE_ENCRYPTED) {
		// todo
	}

	return sectors;
}

void MemoryArchive::extract_file(std::string_view name) {
	auto index = file_lookup(name, 0, 0);

	if(index == npos) {
		throw std::runtime_error("todo");
	}

	const auto btable = fetch_block_table();
	const auto htable = fetch_hash_table();
	auto block_index = htable[index].block_index;
	auto& entry = btable[block_index];

	if(entry.flags & MPQ_FILE_ENCRYPTED) {
		throw std::runtime_error("todo");
	}

	auto sectors = file_sectors(entry);
	auto header = std::bit_cast<const v0::Header*>(buffer_.data());
	const auto max_sector_size = BLOCK_SIZE << header->block_size_shift;

	boost::container::small_vector<unsigned char, 4096> buffer;
	buffer.resize(max_sector_size);

	auto remaining = entry.uncompressed_size;
	auto file = std::fopen(name.data(), "wb"); // todo, check null term
	const auto file_data_offset = buffer_.data() + entry.file_position;

	for(auto i = 0; i < sectors.size() - 1; ++i) {
		std::uint32_t sector_size_actual = max_sector_size;
		std::uint32_t sector_size = max_sector_size;

		// if we're at the end of the file and need to read less
		if(max_sector_size > remaining) {
			sector_size = remaining;
			sector_size_actual = remaining;
		}
		
		// sector is compressed, get the actual data size
		if(entry.flags & MPQ_FILE_COMPRESSED) {
			sector_size_actual = sectors[i + 1] - sectors[i];
		}

		// get the location of the data for this sector
		const auto data = std::bit_cast<unsigned char*>(file_data_offset + sectors[i]);

		if(sector_size_actual < sector_size) {
			if(entry.flags & MPQ_FILE_COMPRESSED) {
				uLongf dest_len = max_sector_size;
				auto ret = uncompress(buffer.data(), &dest_len, data + 1, sector_size_actual);

				if(ret != Z_OK) {
					throw std::runtime_error("todo");
				}

				fwrite(buffer.data(), dest_len, 1, file);
			}
		} else {
			fwrite(data, sector_size_actual, 1, file);
		}

		remaining -= sector_size;
	}
}

std::span<const std::byte> MemoryArchive::retrieve_file(BlockTableEntry& entry) {
	//
	return {};
}

std::span<const BlockTableEntry> MemoryArchive::block_table() const {
	return fetch_block_table();
}

std::span<const HashTableEntry> MemoryArchive::hash_table() const {
	return fetch_hash_table();
}

} // mpq, ember