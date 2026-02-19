/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/MulticharConstant.h>
#include <cstdint>

namespace ember::mpq {

constexpr std::uint16_t header_alignment = 0x200;
constexpr std::uint32_t mpqa_fourcc = utility::make_mcc("MPQ\x1a");
constexpr std::uint32_t mpqb_fourcc = utility::make_mcc("MPQ\x1b");
constexpr std::uint32_t header_size_v0 = 0x20;
constexpr std::uint32_t header_size_v1 = 0x2C;
constexpr std::uint32_t header_size_v2 = 0x2C; // minimum
constexpr std::uint32_t header_size_v3 = 0xD0;
constexpr std::uint32_t mpq_key_hash_table = 0xC3AF3770;
constexpr std::uint32_t mpq_key_block_table = 0xEC83B3A3;
constexpr std::uint32_t mpq_hash_entry_empty = 0xFFFFFFFF;
constexpr std::uint32_t mpq_hash_entry_deleted = 0xFFFFFFFE;
constexpr std::uint32_t mpq_hash_table_index = 0;
constexpr std::uint32_t mpq_hash_name_a = 1;
constexpr std::uint32_t mpq_hash_name_b = 2;
constexpr std::uint32_t mpq_hash_file_key = 3;
constexpr std::uint32_t block_size = 0x200;
constexpr std::uint32_t sector_size_hint = 0x200 << 3;

enum Compression {
	mpq_compression_huffman         = 0x01,
	mpq_compression_zlib            = 0x02,
	mpq_compression_pkware          = 0x08,
	mpq_compression_bzip2           = 0x10,
	mpq_compression_sparse          = 0x20,
	mpq_compression_adpcm_mono      = 0x40,
	mpq_compression_adpcm_stereo    = 0x80,
	mpq_compression_lzma            = 0x12,
	mpq_compression_next_same       = 0xFFFFFFFF,

	mpq_compression_mask = 0xFB
};

constexpr std::uint8_t LZMA_UNCOMPRESSED_SIZE = 8;

} // mpq, ember