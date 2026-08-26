/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <cstdint>

 // todo, need to improve compiler detection
#ifdef _WIN32
#define EMBER_EXPORT __declspec(dllexport)
#else
#define EMBER_EXPORT __attribute__((visibility("default")))
#endif // _WIN32

extern "C" {

EMBER_EXPORT std::uint32_t blaze_api_version_major = 1;
EMBER_EXPORT std::uint32_t blaze_api_version_minor = 0;
EMBER_EXPORT std::uint32_t blaze_api_version_patch = 0;

} // extern "C"

namespace ember::blaze {

namespace impl {
	class Client;
}

class Client {
	std::unique_ptr<impl::Client> impl_;

public:
	Client();
	~Client();
};

} // blaze, ember