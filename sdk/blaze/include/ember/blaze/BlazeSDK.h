/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Shared.h>
#include <memory>
#include <string_view>
#include <cstdint>

inline std::uint32_t blaze_api_version_major = 1;
inline std::uint32_t blaze_api_version_minor = 0;
inline std::uint32_t blaze_api_version_patch = 0;

namespace ember::blaze {

namespace impl {
	class Client;
}

class Client {
	std::unique_ptr<impl::Client> impl_;

public:
	Client();
	~Client();

	void log(LogLevel level, std::string_view message);
	void slog(LogLevel level, std::string_view message);
};

} // blaze, ember