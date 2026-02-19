/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/SharedDefs.h>
#include <shared/utility/enum_bitmask.h>
#include <cstdint>

namespace ember::mpq {

enum Flags : std::uint32_t {
	mpq_file_implode       = 0x00000100, // PKWARE compression
	mpq_file_compress      = 0x00000200,
	mpq_file_encrypted     = 0x00010000,
	mpq_file_fix_key       = 0x00020000,
	mpq_file_patch_file    = 0x00100000,
	mpq_file_single_unit   = 0x01000000,
	mpq_file_delete_marker = 0x02000000,
	mpq_file_sector_crc    = 0x04000000,
	mpq_file_exists        = 0x80000000,
	mpq_file_compress_mask = 0x0000FF00  // StormLib defines as 0xFF but why?
};

enum class Locale : std::uint16_t {
	neutral         = 0x00,
	taiwan_mandarin = 0x404,
	german          = 0x407,
	spanish         = 0x40a,
	italian         = 0x410,
	korean          = 0x412,
	portugese       = 0x416,
	czech           = 0x405,
	english_us      = 0x409,
	french          = 0x40c,
	japanese        = 0x411,
	polish          = 0x415,
	russian         = 0x419,
	english_uk      = 0x809
};

namespace v0 {

struct Header {
	std::uint32_t magic;
	std::uint32_t header_size;
	std::uint32_t archive_size;
	std::uint16_t format_version;
	std::uint16_t block_size_shift;
	std::uint32_t hash_table_offset;
	std::uint32_t block_table_offset;
	std::uint32_t hash_table_size;
	std::uint32_t block_table_size;
};

} // v0

namespace v1 {

struct Header : public v0::Header {
	std::uint64_t extended_block_table_offset;
	std::uint16_t hash_table_offset_hi;
	std::uint16_t block_table_offset_hi;
	std::uint32_t __pad;
};

} // v1

struct UserDataHeader {
	std::uint32_t magic;
	std::uint32_t user_data_size;
	std::uint32_t header_offset;
};

static_assert(sizeof(UserDataHeader) == 0x0C);

struct HashTableEntry {
	std::uint32_t name_1;
	std::uint32_t name_2;
	std::uint16_t locale;
	std::uint16_t platform;
	std::uint32_t block_index;
};

static_assert(sizeof(HashTableEntry) == 0x10);

struct BlockTableEntry {
	std::uint32_t file_position;
	std::uint32_t compressed_size;
	std::uint32_t uncompressed_size;
	Flags flags;
};

static_assert(sizeof(BlockTableEntry) == 0x10);

} // mpq, ember
