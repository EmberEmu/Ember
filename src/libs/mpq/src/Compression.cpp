/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/Compression.h>
#include <mpq/Exception.h>
#include <mpq/SharedDefs.h>
#include <zlib.h>
#include <bzip2/bzlib.h>
#include <lzma/LzmaLib.h>
#include <adpcm/adpcm.h>
#include <sparse/sparse.h>
#include <huffman/huff.h>
#include <boost/container/small_vector.hpp>
#include <bit>

namespace ember::mpq {

std::expected<std::size_t, int> decompress_huffman(std::span<const std::byte> input,
                                                   std::span<std::byte> output) {
	auto dest = reinterpret_cast<unsigned char*>(output.data());
	auto dest_len = static_cast<int>(output.size_bytes());

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
	auto src = reinterpret_cast<const unsigned char*>(input.data());
	auto dest = reinterpret_cast<unsigned char*>(output.data());
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
	auto src = reinterpret_cast<const unsigned char*>(input.data());
	auto dest = reinterpret_cast<unsigned char*>(output.data());

	return DecompressADPCM(dest, output.size_bytes(), src, input.size_bytes(), channels);
}

std::expected<std::size_t, int> decompress_lzma(std::span<const std::byte> input,
                                                std::span<std::byte> output) {
	constexpr auto header_size = LZMA_PROPS_SIZE + LZMA_UNCOMPRESSED_SIZE;

	auto dest_len = output.size_bytes();
	auto dest = reinterpret_cast<unsigned char*>(output.data());
	auto src = reinterpret_cast<const unsigned char*>(input.data());
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
	auto dest = reinterpret_cast<char*>(output.data());
	auto src = reinterpret_cast<const char*>(input.data());

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
	auto dest = reinterpret_cast<Bytef*>(output.data());
	auto src = reinterpret_cast<const Bytef*>(input.data());

	auto ret = uncompress(dest, &dest_len, src + 1, input.size_bytes());

	if(ret != Z_OK) {
		return std::unexpected(ret);
	} else {
		return dest_len;
	}
}

int next_compression(std::uint8_t& mask) {
	if(!mask) {
		return 0;
	}

	if(mask == mpq_compression_lzma) {
		mask = 0;
		return mpq_compression_lzma;
	}

	if(mask & mpq_compression_huffman) {
		mask ^= mpq_compression_huffman;
		return mpq_compression_huffman;
	}

	if(mask & mpq_compression_adpcm_mono) {
		mask ^= mpq_compression_adpcm_mono;
		return mpq_compression_adpcm_mono;
	}

	if(mask & mpq_compression_adpcm_stereo) {
		mask ^= mpq_compression_adpcm_stereo;
		return mpq_compression_adpcm_stereo;
	}

	if(mask & mpq_compression_sparse) {
		mask ^= mpq_compression_sparse;
		return mpq_compression_sparse;
	}

	if(mask & mpq_compression_bzip2) {
		mask ^= mpq_compression_bzip2;
		return mpq_compression_bzip2;
	}

	if(mask & mpq_compression_pkware) {
		mask ^= mpq_compression_pkware;
		return mpq_compression_pkware;
	}

	if(mask & mpq_compression_zlib) {
		mask ^= mpq_compression_zlib;
		return mpq_compression_zlib;
	}

	return -1;
}

std::expected<std::size_t, int> do_decompression(std::span<const std::byte> input,
                                                 std::span<std::byte> output,
                                                 const int comp) {
	switch(comp) {
		case mpq_compression_huffman:
			return decompress_huffman(input, output);
		case mpq_compression_sparse:
			return decompress_sparse(input, output);
		case mpq_compression_adpcm_mono:
			return decompress_adpcm(input, output, 1);
		case mpq_compression_adpcm_stereo:
			return decompress_adpcm(input, output, 2);
		case mpq_compression_lzma:
			return decompress_lzma(input, output);
		case mpq_compression_bzip2:
			return decompress_bzip2(input, output);
		case mpq_compression_pkware:
			return decompress_pklib({ input.data() + 1, input.size_bytes() }, output);
		case mpq_compression_zlib:
			return decompress_zlib(input, output);
		default:
			throw unknown_format();
	}
}

std::expected<std::size_t, DecompressionError> decompress(std::span<const std::byte> input,
                                                          std::span<std::byte> output,
                                                          int def_comp) {
	std::uint8_t comp_mask = std::bit_cast<std::uint8_t>(input[0]);
	std::expected<std::size_t, int> result;
	int prev = 0;
	std::size_t prev_size = 0;

	while(auto comp = next_compression(comp_mask)) try {
		if(comp == mpq_compression_next_same) {
			comp = def_comp;
		}

		if(!prev) {
			result = do_decompression(input, output, comp);
		} else {
			boost::container::small_vector<std::byte, sector_size_hint> buffer(
				prev_size, boost::container::default_init
			);

			std::memcpy(buffer.data(), output.data(), prev_size);
			result = do_decompression(buffer, output, comp);
		}

		if(!result) {
			const DecompressionError error {
				.unknown = false,
				.compression = static_cast<Compression>(comp),
				.error = result.error()

			};

			return std::unexpected(error);
		} else {
			prev_size = result.value();
		}

		prev = comp;
	} catch(const unknown_format&) {
		const DecompressionError error {
			.unknown = true
		};

		return std::unexpected(error);
	}

	return result.value();
}

} // mpq, ember