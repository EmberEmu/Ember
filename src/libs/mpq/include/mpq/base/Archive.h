/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/Structures.h>
#include <filesystem>
#include <span>
#include <string_view>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::mpq {

class ExtractionSink;

class Archive {
public:
	static constexpr std::size_t npos = -1;
	
	enum class Backing {
		FILE, MAPPED, MEMORY
	};

	virtual int version() const = 0;
	virtual std::size_t size() const = 0;
	virtual Backing backing() const = 0;
	virtual std::span<const BlockTableEntry> block_table() const = 0;
	virtual std::span<const HashTableEntry> hash_table() const = 0;
	virtual std::size_t file_lookup(std::string_view name, const std::uint16_t locale) const = 0;
	virtual void extract_file(const std::filesystem::path& path, ExtractionSink& store) = 0;
	virtual std::span<const std::string> files() const = 0;
	virtual void files(std::span<std::string_view> files) = 0;

	virtual ~Archive() = default;
};

} // mpq, ember