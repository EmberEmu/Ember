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
#include <bit>
#include <zlib.h>
#include <bzip2/bzlib.h>
#include <lzma/LzmaLib.h>
#include <adpcm/adpcm.h>
#include <sparse/sparse.h>
#include <huffman/huff.h>

namespace ember::mpq {

std::expected<std::size_t, int> decompress_huffman(std::span<const std::byte> input,
                                                   std::span<std::byte> output) {
	auto src = std::bit_cast<const unsigned char*>(input.data());
	auto dest = std::bit_cast<unsigned char*>(output.data());
	int dest_len = static_cast<int>(output.size_bytes());

	THuffmannTree ht(false);
	TInputStream is(input.data() + 1, input.size_bytes() - 1);

	dest_len = ht.Decompress(dest, dest_len, &is);

	if(!dest_len) {
		return std::unexpected(0);
	} else {
		return dest_len;
	}
}

std::expected<std::size_t, int> decompress_sparse(std::span<const std::byte> input,
                                                  std::span<std::byte> output) {
	auto src = std::bit_cast<unsigned char*>(input.data());
	auto dest = std::bit_cast<unsigned char*>(output.data());
	int dest_len = static_cast<int>(output.size_bytes());

	auto res = DecompressSparse(dest, &dest_len, src + 1, input.size_bytes() - 1);

	if(!res) {
		return std::unexpected(res);
	} else {
		return dest_len;
	}
}

std::expected<std::size_t, int> decompress_adpcm(std::span<const std::byte> input,
                                                 std::span<std::byte> output,
                                                 const int channels) {
	auto src = std::bit_cast<unsigned char*>(input.data());
	auto dest = std::bit_cast<unsigned char*>(output.data());

	return DecompressADPCM(dest, output.size_bytes(), src + 1, input.size_bytes() - 1, channels);
}

std::expected<std::size_t, int> decompress_lzma(std::span<const std::byte> input,
                                                std::span<std::byte> output) {
	constexpr auto header_size = LZMA_PROPS_SIZE + LZMA_UNCOMPRESSED_SIZE;

	auto dest_len = output.size_bytes();
	auto dest = std::bit_cast<unsigned char*>(output.data());
	auto src = std::bit_cast<const unsigned char*>(input.data());
	auto src_len = output.size_bytes() - header_size;

	auto ret = LzmaUncompress(dest, &dest_len, src + header_size, &src_len, src + 1, LZMA_PROPS_SIZE);

	if(ret != SZ_OK) {
		return std::unexpected(ret);
	} else {
		return dest_len;
	}
}
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

	auto ret = uncompress(dest, &dest_len, src + 1, input.size_bytes());

	if(ret != Z_OK) {
		return std::unexpected(ret);
	} else {
		return dest_len;
	}
}

// this is very rough and does not work as it should yet, will fix as required
int next_compression(std::uint8_t& mask) {
	if(!mask) {
		return 0;
	}

	if(mask == MPQ_COMPRESSION_LZMA) {
		mask = 0;
		return MPQ_COMPRESSION_LZMA;
	}

	if(mask & MPQ_COMPRESSION_ADPCM_MONO) {
		mask ^= MPQ_COMPRESSION_ADPCM_MONO;
		return MPQ_COMPRESSION_ADPCM_MONO;
	}

	if(mask & MPQ_COMPRESSION_ADPCM_STEREO) {
		mask ^= MPQ_COMPRESSION_ADPCM_STEREO;
		return MPQ_COMPRESSION_ADPCM_STEREO;
	}

	if(mask & MPQ_COMPRESSION_ADPCM_STEREO) {
		mask ^= MPQ_COMPRESSION_ADPCM_STEREO;
		return MPQ_COMPRESSION_ADPCM_STEREO;
	}

	if(mask & MPQ_COMPRESSION_HUFFMANN) {
		mask ^= MPQ_COMPRESSION_HUFFMANN;
		return MPQ_COMPRESSION_HUFFMANN;
	}

	if(mask & MPQ_COMPRESSION_SPARSE) {
		mask ^= MPQ_COMPRESSION_SPARSE;
		return MPQ_COMPRESSION_SPARSE;
	}

	if(mask & MPQ_COMPRESSION_BZIP2) {
		mask ^= MPQ_COMPRESSION_BZIP2;
		return MPQ_COMPRESSION_BZIP2;
	}

	if(mask & MPQ_COMPRESSION_PKWARE) {
		mask ^= MPQ_COMPRESSION_PKWARE;
		return MPQ_COMPRESSION_PKWARE;
	}

	if(mask & MPQ_COMPRESSION_ZLIB) {
		mask ^= MPQ_COMPRESSION_ZLIB;
		return MPQ_COMPRESSION_ZLIB;
	}

	return -1;
}

std::expected<std::size_t, int> decompress(std::span<const std::byte> input,
                                           std::span<std::byte> output) {
	std::uint8_t comp_mask = std::bit_cast<std::uint8_t>(input[0]);
	std::expected<std::size_t, int> result;
	std::uint8_t prev = 0;

	while(auto comp = next_compression(comp_mask)) {
		if(comp == MPQ_COMPRESSION_NEXT_SAME) {
			comp = prev;
		}

		switch(comp) {
			case MPQ_COMPRESSION_HUFFMANN:
				result = decompress_huffman(input, output);
				break;
			case MPQ_COMPRESSION_SPARSE:
				result = decompress_sparse(input, output);
				break;
			case MPQ_COMPRESSION_ADPCM_MONO:
				result = decompress_adpcm(input, output, 1);
				break;
			case MPQ_COMPRESSION_ADPCM_STEREO:
				result = decompress_adpcm(input, output, 2);
				break;
			case MPQ_COMPRESSION_LZMA:
				result = decompress_lzma(input, output);
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
			default:
				throw exception("decompression: unknown type");
		}

		if(!result) {
			return result;
		}

		prev = comp;
	}

	return result;
}

} // mpq, ember