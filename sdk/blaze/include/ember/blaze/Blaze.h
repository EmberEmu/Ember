/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

inline std::uint32_t blaze_api_version_major = 1;
inline std::uint32_t blaze_api_version_minor = 0;
inline std::uint32_t blaze_api_version_patch = 0;

namespace ember::blaze {

namespace impl {
	class Client;
}

class Client {
public:
	Client() {

	}

	~Client() {

	}

	void log(int level, const char* message) {

	}

	void slog(int level, const char* message) {

	}
};

} // blaze, ember