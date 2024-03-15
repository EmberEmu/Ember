/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/util/MulticharConstant.h>
#include <cstdint>

namespace ember::mpq {

constexpr std::uint16_t HEADER_ALIGNMENT = 0x200;
constexpr std::uint32_t MPQA_FOURCC = util::make_mcc("MPQ\x1a");
constexpr std::uint32_t MPQB_FOURCC = util::make_mcc("MPQ\x1b");
constexpr std::uint32_t HEADER_SIZE_V0 = 0x20;
constexpr std::uint32_t HEADER_SIZE_V1 = 0x2C;
constexpr std::uint32_t HEADER_SIZE_V2 = 0x2C; // minimum
constexpr std::uint32_t HEADER_SIZE_V3 = 0xD0;
constexpr std::uint32_t KEY_HASH_TABLE = 0xC3AF3770;
constexpr std::uint32_t KEY_BLOCK_TABLE = 0xEC83B3A3;

enum Flags : std::uint32_t {
	MPQ_FILE_IMPLODE       = 0x00000100, // PKWARE compression
	MPQ_FILE_COMPRESS      = 0x00000200,
	MPQ_FILE_COMPRESSED    = 0x0000FF00,
	MPQ_FILE_ENCRYPTED     = 0x00010000,
	MPQ_FILE_FIX_KEY       = 0x00020000,
	MPQ_FILE_PATCH_FILE    = 0x00100000,
	MPQ_FILE_SINGLE_UNIT   = 0x01000000,
	MPQ_FILE_DELETE_MARKER = 0x02000000,
	MPQ_FILE_SECTOR_CRC    = 0x04000000,
	MPQ_FILE_EXISTS        = 0x80000000,
};

enum class Locale : std::uint16_t {
	NEUTRAL         = 0x00,
	TAIWAN_MANDARIN = 0x404,
	GERMAN          = 0x407,
	SPANISH         = 0x40a,
	ITALIAN         = 0x410,
	KOREAN          = 0x412,
	PORTUGESE       = 0x416,
	CZECH           = 0x405,
	ENGLISH_US      = 0x409,
	FRENCH          = 0x40c,
	JAPANESE        = 0x411,
	POLISH          = 0x415,
	RUSSIAN         = 0x419,
	ENGLISH_UK      = 0x809
};

namespace v0 {

struct Header {
	std::uint32_t magic;
	std::uint32_t header_size;
	std::uint32_t archive_size;
	std::uint16_t format_version;
	std::uint16_t block_size;
	std::uint32_t hash_table_offset;
	std::uint32_t block_table_offset;
	std::uint32_t hash_table_size;
	std::uint32_t block_table_size;
};

} // v0

namespace v1 {

struct Header {
	std::uint32_t magic;
	std::uint32_t header_size;
	std::uint32_t archive_size;
	std::uint16_t format_version;
	std::uint16_t block_size;
	std::uint32_t hash_table_offset;
	std::uint32_t block_table_offset;
	std::uint32_t hash_table_size;
	std::uint32_t block_table_size;
	std::uint64_t extended_block_table_offset;
	std::uint16_t hash_table_offset_high;
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