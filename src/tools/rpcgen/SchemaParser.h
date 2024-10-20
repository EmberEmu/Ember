/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <flatbuffers/reflection_generated.h>
#include <filesystem>
#include <span>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember {

class SchemaParser final {
	std::filesystem::path tpl_path_;
	std::filesystem::path out_path_;

	void verify(std::span<const std::uint8_t> buffer);
	std::vector<std::uint8_t> load_file(const std::filesystem::path& path);
	void process(const reflection::Schema* schema, const reflection::Service* service);

public:
	SchemaParser(std::filesystem::path templates_dir, std::filesystem::path output_dir);

	void generate(const std::filesystem::path& path);
	void generate(std::span<const std::uint8_t> buffer);
};

} // ember