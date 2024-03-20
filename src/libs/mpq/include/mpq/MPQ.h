/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/Archive.h>
#include <mpq/Structures.h>
#include <expected>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <cstdint>

namespace ember::mpq {

static constexpr std::uintptr_t npos = -1;

enum class ErrorCode {
	SUCCESS,
	NO_ARCHIVE_FOUND,
	BAD_ALIGNMENT,
	FILE_NOT_FOUND,
	UNABLE_TO_OPEN,
	FILE_READ_FAILED
};

using LocateResult = std::expected<std::uintptr_t, ErrorCode>;

LocateResult locate_archive(const std::filesystem::path& path);
LocateResult locate_archive(std::span<const std::byte> data);

std::unique_ptr<Archive> open_archive(const std::filesystem::path& path,
                                      std::uintptr_t offset,
                                      bool map);

std::unique_ptr<MemoryArchive> open_archive(std::span<std::byte> data,
                                      std::uintptr_t offset = 0);

std::unique_ptr<FileArchive> create_archive(std::uint32_t version);
bool validate_header(const v0::Header& header);

} // mpq, ember