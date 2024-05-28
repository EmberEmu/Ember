/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "grunt/Magic.h"
#include "shared/util/FNVHash.h"
#include <optional>
#include <string>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember {

class IntegrityData final {
	struct Key {
		std::uint16_t build;
		grunt::Platform platform;
		grunt::System os;

		bool operator==(const Key& key) const {
			return key.build == build
				&& key.platform == platform
				&& key.os == os;
		}
	};

	struct KeyHash {
		std::size_t operator()(const Key& key) const {
			FNVHash hasher;
			hasher.update(key.build);
			hasher.update(key.platform);
			return hasher.update(key.os);
		}
	};

	std::unordered_map<Key, std::vector<std::byte>, KeyHash> data_;

	void load_binaries(std::string_view path, std::uint16_t build,
	                   std::span<std::string_view> files,
	                   grunt::System system, grunt::Platform platform);

public:
	IntegrityData(std::span<const GameVersion> versions, std::string_view path);

	std::optional<std::span<const std::byte>> lookup(GameVersion version,
	                                                 grunt::Platform platform,
	                                                 grunt::System os) const;
};


} // ember