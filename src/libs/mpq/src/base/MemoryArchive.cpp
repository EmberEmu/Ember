/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/base/MemoryArchive.h>
#include <mpq/Compression.h>
#include <mpq/Crypt.h>
#include <mpq/Exception.h>
#include <mpq/Structures.h>
#include <mpq/MemorySink.h>
#include <boost/endian/conversion.hpp>
#include <boost/container/small_vector.hpp>
#include <bit>
#include <vector>
#include <zlib.h> // todo
#include <iostream> // todo
#include <fstream>
#include <cmath>

namespace ember::mpq {

MemoryArchive::MemoryArchive(std::span<std::byte> buffer)
	: buffer_(buffer),
	  header_(std::bit_cast<const v0::Header*>(buffer_.data())) {
	block_table_ = fetch_block_table();
	hash_table_ = fetch_hash_table();
	decrypt_block(std::as_writable_bytes(block_table_), MPQ_KEY_BLOCK_TABLE);
	decrypt_block(std::as_writable_bytes(hash_table_), MPQ_KEY_HASH_TABLE);
	load_listfile();
}

void MemoryArchive::load_listfile() {
	auto index = file_lookup("(listfile)", 0, 0);

	if(index == npos) {
		return;
	}

	const auto& entry = file_entry(index);
	std::vector<std::byte> buffer;
	buffer.resize(entry.uncompressed_size);
	MemorySink sink(buffer);
	extract_file("(listfile)", sink);
}

int MemoryArchive::version() const { 
	return boost::endian::little_to_native(header_->format_version);
}

std::size_t MemoryArchive::size() const {
	return boost::endian::little_to_native(header_->archive_size);
}

std::span<HashTableEntry> MemoryArchive::fetch_hash_table() const {
	auto entry = std::bit_cast<HashTableEntry*>(
		buffer_.data() + header_->hash_table_offset
	);

	return { entry, header_->hash_table_size };
}

std::span<BlockTableEntry> MemoryArchive::fetch_block_table() const {
	auto entry = std::bit_cast<BlockTableEntry*>(
		buffer_.data() + header_->block_table_offset
	);

	return { entry, header_->block_table_size };
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

std::span<std::uint32_t> MemoryArchive::file_sectors(const BlockTableEntry& entry) {
	const auto sector_size = BLOCK_SIZE << header_->block_size_shift;
	auto count = (uint32_t)std::floor(entry.uncompressed_size / sector_size + 1); // todo

	if(entry.flags & Flags::MPQ_FILE_SECTOR_CRC) {
		++count;
	}

	auto sector_begin = std::bit_cast<std::uint32_t*>(buffer_.data() + entry.file_position);
	return { sector_begin, count + 1 };
}

void MemoryArchive::extract_file(std::filesystem::path path, ExtractionSink& sink) {
	auto index = file_lookup(path.string(), 0, 0);

	if(index == npos) {
		throw exception("cannot extract file: file not found");
	}

	const auto filename = path.filename().string();
	auto& entry = file_entry(index);
	auto max_sector_size = BLOCK_SIZE << header_->block_size_shift;
	const auto file_offset = buffer_.data() + entry.file_position;
	auto sectors = file_sectors(entry);
	const auto key = hash_string(filename, MPQ_HASH_FILE_KEY);

	// decrypt the sector block if required
	if(entry.flags & Flags::MPQ_FILE_ENCRYPTED) {
		decrypt_block(std::as_writable_bytes(sectors), key - 1);
	}

	std::size_t ignore_count = 1;

	if(entry.flags & Flags::MPQ_FILE_SECTOR_CRC) {
		++ignore_count;
	}

	sectors = std::span(sectors.begin(), sectors.end() - ignore_count);
	auto remaining = entry.uncompressed_size;

	boost::container::small_vector<unsigned char, 4096> buffer;
	buffer.resize(max_sector_size);

	for(std::size_t i = 0u; i < sectors.size(); ++i) {
		std::uint32_t sector_size_actual = max_sector_size;
		std::uint32_t sector_size = max_sector_size;

		// if we're at the end of the file and need to read less
		if(max_sector_size > remaining) {
			sector_size = remaining;
			sector_size_actual = remaining;
		}
		
		// sector is compressed, get the actual data size
		if(entry.flags & Flags::MPQ_FILE_COMPRESS_MASK) {
			sector_size_actual = sectors[i + 1] - sectors[i];
		}

		// get the location of the data for this sector
		std::span sector_data(
			std::bit_cast<unsigned char*>(file_offset + sectors[i]), sector_size_actual
		);

		if(entry.flags & Flags::MPQ_FILE_ENCRYPTED) {
			decrypt_block(std::as_writable_bytes(sector_data), key + i);
		}

		if(sector_size_actual < sector_size) {
			if(entry.flags & Flags::MPQ_FILE_COMPRESS) {
				uLongf dest_len = max_sector_size;
				auto ret = uncompress(buffer.data(), &dest_len, sector_data.data() + 1, sector_data.size());

				if(ret != Z_OK) {
					throw exception("cannot extract file: decompression failed");
				}

				sink.store(std::as_bytes(std::span(buffer.data(), dest_len)));
			} else if(entry.flags & Flags::MPQ_FILE_IMPLODE) {
				auto ret = decompress_pklib(
					std::as_bytes(sector_data), std::as_writable_bytes(std::span(buffer))
				);

				if(!ret) {
					throw exception("cannot extract file: decompression (explode) failed");
				}

				sink.store(std::as_bytes(std::span(buffer.data(), *ret)));
			}
		} else {
			sink.store(std::as_bytes(sector_data));
		}

		remaining -= sector_size;
	}

	entry.flags = static_cast<Flags>(entry.flags ^ Flags::MPQ_FILE_ENCRYPTED);
}

std::span<const std::byte> MemoryArchive::retrieve_file(BlockTableEntry& entry) {
	//
	return {};
}

BlockTableEntry& MemoryArchive::file_entry(const std::size_t index) {
	auto block_index = hash_table_[index].block_index;
	return block_table_[block_index];
}

std::span<const BlockTableEntry> MemoryArchive::block_table() const {
	return block_table_;
}

std::span<const HashTableEntry> MemoryArchive::hash_table() const {
	return hash_table_;
}

} // mpq, ember