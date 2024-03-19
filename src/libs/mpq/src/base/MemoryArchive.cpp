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
#include <iterator>
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

	std::string buffer;
	buffer.resize(entry.uncompressed_size);
	MemorySink sink(std::as_writable_bytes(std::span(buffer)));
	extract_file("(listfile)", sink);

	std::stringstream stream;
	stream << buffer;
	std::string().swap(buffer); // force memory release

	while(std::getline(stream, buffer, '\n')) {
		std::erase(buffer, '\r');
		files_.emplace_back(std::move(buffer));
		buffer.clear(); // reset to known state
	}
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

	for(auto i = index; i != end; ++i, i %= table.size()) {
		if(table[i].block_index == MPQ_HASH_ENTRY_EMPTY) {
			return npos;
		}

		if(table[i].name_1 == name_key_a && table[i].name_2 == name_key_b
		   && table[i].locale == locale && table[i].platform == platform) {
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

void MemoryArchive::extract_file(const std::filesystem::path& path, ExtractionSink& store) {
	auto index = file_lookup(path.string(), 0, 0);

	if(index == npos) {
		throw exception("cannot extract file: file not found");
	}

	const auto filename = path.filename().string();
	auto& entry = file_entry(index);
	auto max_sector_size = BLOCK_SIZE << header_->block_size_shift;
	const auto file_offset = buffer_.data() + entry.file_position;
	auto sectors = file_sectors(entry);
	auto key = hash_string(filename, MPQ_HASH_FILE_KEY);

	// decrypt the sector block if required
	if(entry.flags & Flags::MPQ_FILE_ENCRYPTED) {
		decrypt_block(std::as_writable_bytes(sectors), --key);
	}

	std::size_t ignore_count = 1;

	if(entry.flags & Flags::MPQ_FILE_SECTOR_CRC) {
		++ignore_count;
	}

	sectors = std::span(sectors.begin(), sectors.end() - ignore_count);
	auto remaining = entry.uncompressed_size;

	boost::container::small_vector<std::byte, LIKELY_SECTOR_SIZE> buffer;
	buffer.resize(max_sector_size);

	if(entry.flags & MPQ_FILE_FIX_KEY) {
		throw exception("todo");
	}

	for(auto sector = sectors.begin(); sector != sectors.end(); ++sector) {
		std::uint32_t sector_size_actual = max_sector_size;
		std::uint32_t sector_size = max_sector_size;

		// if we're at the end of the file and need to read less
		if(max_sector_size > remaining) {
			sector_size = remaining;
			sector_size_actual = remaining;
		}
		
		// sector is compressed, get the actual data size
		if(entry.flags & Flags::MPQ_FILE_COMPRESS_MASK) {
			sector_size_actual =  *std::next(sector) - *sector;
		}

		// get the location of the data for this sector
		std::span sector_data(
			std::bit_cast<unsigned char*>(file_offset + *sector), sector_size_actual
		);

		if(entry.flags & Flags::MPQ_FILE_ENCRYPTED) {
			decrypt_block(std::as_writable_bytes(sector_data), ++key);
		}

		if(sector_size_actual < sector_size) {
			if(entry.flags & Flags::MPQ_FILE_COMPRESS) {
				auto ret = decompress(
					std::as_bytes(sector_data), std::as_writable_bytes(std::span(buffer))
				);

				if(!ret) {
					throw exception("cannot extract file: decompression failed");
				}

				store(std::as_bytes(std::span(buffer.data(), *ret)));
			} else if(entry.flags & Flags::MPQ_FILE_IMPLODE) {
				auto ret = decompress_pklib(
					std::as_bytes(sector_data), std::as_writable_bytes(std::span(buffer))
				);

				if(!ret) {
					throw exception("cannot extract file: decompression (explode) failed");
				}

				store(std::as_bytes(std::span(buffer.data(), *ret)));
			}
		} else {
			store(std::as_bytes(sector_data));
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

std::span<const std::string> MemoryArchive::files() const {
	return files_;
}

} // mpq, ember