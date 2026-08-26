/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Shared.h>
#include <string_view>

namespace ember::blaze::impl {

class Client final {
	void resolve_symbols();

public:
	Client();
	~Client();

	void log(LogLevel level, std::string_view message);
	void slog(LogLevel level, std::string_view message);
};

} // impl, blaze, ember