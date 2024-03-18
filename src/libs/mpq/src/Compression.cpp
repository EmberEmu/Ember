/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/Compression.h>
#include <mpq/Exception.h>
#include <mpq/SharedDefs.h>
#include <zlib.h>
#include <bzip2/bzlib.h>

namespace ember::mpq {

std::expected<std::size_t, int> decompress_bzip2(std::span<const std::byte> input,
                                                 std::span<std::byte> output) {
	unsigned int dest_len = output.size_bytes();
	char* dest = std::bit_cast<char*>(output.data());
	const char* src = std::bit_cast<const char*>(input.data());

	auto ret = BZ2_bzBuffToBuffDecompress(dest, &dest_len, src + 1, input.size_bytes() - 1, 0, 0);

	if(ret != BZ_OK) {
		return std::unexpected(ret);
	} else {
		return dest_len;
	}
}

std::expected<std::size_t, int> decompress_zlib(std::span<const std::byte> input,
                                                std::span<std::byte> output) {
	uLongf dest_len = output.size_bytes();
	Bytef* dest = std::bit_cast<Bytef*>(output.data());
	const Bytef* src = std::bit_cast<const Bytef*>(input.data());

	auto ret = uncompress(dest, &dest_len, src + 1, input.size_bytes() - 1);

	if(ret != Z_OK) {
		return std::unexpected(ret);
	} else {
		return dest_len;
	}
}

std::expected<std::size_t, int> decompress(std::span<const std::byte> input,
                                           std::span<std::byte> output) {
	std::uint8_t compression = std::bit_cast<std::uint8_t>(input[0]);
	std::expected<std::size_t, int> result;

	switch(compression) {
		case MPQ_COMPRESSION_LZMA:
		case MPQ_COMPRESSION_ADPCM_MONO:
		case MPQ_COMPRESSION_ADPCM_STEREO:
		case MPQ_COMPRESSION_HUFFMANN:
		case MPQ_COMPRESSION_NEXT_SAME:
		case MPQ_COMPRESSION_SPARSE:
			throw exception("decompression: unsupported type");
			break;
		case MPQ_COMPRESSION_BZIP2:
			result = decompress_bzip2(input, output);
			break;
		case MPQ_COMPRESSION_PKWARE:
			result = decompress_pklib(input, output);
			break;
		case MPQ_COMPRESSION_ZLIB:
			result = decompress_zlib(input, output);
			break;
	}

	return result;
}

} // mpq, ember