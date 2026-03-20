/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ArgumentValue.h>

namespace ember::commands {

class VariantArg {
	const commands::args::Value value_;

public:
	VariantArg(commands::args::Value value)
		: value_(std::move(value)) {}

	template<typename _ty>
	const _ty& as() const {
		return std::get<_ty>(value_);
	}
};

} // commands, ember