/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpq/MPQ.h>
#include <mpq/Structures.h>
#include <mpq/Archive.h>
#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/buffers.hpp>
#include <bit>
#include <fstream>
#include <cstddef>

using namespace boost::interprocess;

namespace ember::mpq {

std::uintptr_t archive_offset(std::span<const std::byte> buffer) {
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	std::uint32_t magic{};

	for(std::uintptr_t i = 0; i < buffer.size(); i += HEADER_ALIGNMENT) {
		stream >> magic;
		boost::endian::big_to_native_inplace(magic);

		if(magic == MPQA_FOURCC) {
			return i;
		}

		if(stream.size() > HEADER_ALIGNMENT) {
			stream.skip(HEADER_ALIGNMENT - sizeof(magic));
		}
	}

	return npos;
}

LocateResult locate_archive(const std::filesystem::path& path) try {
	if(!std::filesystem::exists(path)) {
		return std::unexpected(ErrorCode::FILE_NOT_FOUND);
	}

	file_mapping file(path.c_str(), read_only);
	mapped_region region(file, read_only);
	const auto data = region.get_address();
	const auto size = region.get_size();
	region.advise(mapped_region::advice_sequential);
	return locate_archive({ static_cast<std::byte*>(data), size });
} catch(std::exception&) {
	return std::unexpected(ErrorCode::UNABLE_TO_OPEN);
}

LocateResult locate_archive(std::span<const std::byte> buffer) {
	const auto address = std::bit_cast<std::uintptr_t>(buffer.data());

	if(address % alignof(v0::Header)) {
		return std::unexpected(ErrorCode::BAD_ALIGNMENT);
	}

	const auto offset = archive_offset(buffer);

	if(offset == npos) {
		return std::unexpected(ErrorCode::NO_ARCHIVE_FOUND);
	}

	return offset;
}

std::unique_ptr<Archive> open_archive(const std::filesystem::path& path,
									  const std::uintptr_t offset,
									  const bool map) {
	std::error_code ec{};

	if(!std::filesystem::exists(path, ec) || ec) {
		throw std::runtime_error("todo");
	}

	std::ifstream stream;
	stream.open(path, std::ios::binary | std::ios::in);

	if(!stream.is_open()) {
		throw std::runtime_error("todo");
	}

	std::array<char, sizeof(v0::Header)> h_buf{};
	stream.seekg(offset, std::ios::beg);
	stream.read(h_buf.data(), h_buf.size());

	if(!stream.good()) {
		throw std::runtime_error("todo");
	}

	stream.close();

	const auto header_v0 = std::bit_cast<const v0::Header*>(h_buf.data());

	if(!validate_header(*header_v0)) {
		throw std::runtime_error("todo");
	}

	if(header_v0->format_version == 0) {
		if(map) {
			file_mapping file(path.c_str(), read_write);
			mapped_region region(file, copy_on_write);
			region.advise(mapped_region::advice_sequential);
			return std::make_unique<v0::MappedArchive>(std::move(file), std::move(region));
		} else {
			return std::make_unique<v0::FileArchive>(path, offset);
		}
	}

	return nullptr;
}

std::unique_ptr<MemoryArchive> open_archive(std::span<std::byte> data,
                                            const std::uintptr_t offset) {
	const auto header_v0 = std::bit_cast<const v0::Header*>(data.data() + offset);
	auto adjusted = std::span(data.data() + offset, data.size_bytes() - offset);

	if(header_v0->format_version == 0) {
		return std::make_unique<v0::MemoryArchive>(adjusted);
	}

	return nullptr;
}

std::unique_ptr<mpq::FileArchive> create_archive(const std::uint32_t version) {
	return nullptr;
}

// todo
bool validate_header(const v0::Header& header) {
	if(boost::endian::native_to_big(header.magic) != MPQA_FOURCC) {
		return false;
	}

	if(header.format_version == 0) {
		if(header.header_size != HEADER_SIZE_V0) {
			return false;
		}
	} else if(header.format_version == 1) {
		if(header.header_size != HEADER_SIZE_V1) {
			return false;
		}
	} else if(header.format_version == 2) {
		if(header.header_size >= HEADER_SIZE_V2) {
			return false;
		}
	} else if(header.format_version == 3) {
		if(header.header_size != HEADER_SIZE_V3) {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

} // mpq, ember