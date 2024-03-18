/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <span>
#include <cstddef>

namespace ember::mpq {

class ExtractionSink {
public:
	virtual void store(std::span<const std::byte> data) = 0;
	virtual ~ExtractionSink() = default;
	virtual void operator()(std::span<const std::byte> data) = 0;
};

} // mpq, ember