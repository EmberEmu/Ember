/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::realm {

template<typename T>
class BindToken {
	const T* instance_;

	explicit BindToken(const T* instance)
		: instance_(instance) {}

	friend T;

public:
	BindToken(const BindToken&) = delete;
	BindToken& operator=(const BindToken&) = delete;
	BindToken(BindToken&&) noexcept = default;
	BindToken& operator=(BindToken&&) = delete;

	const T* bound() const noexcept {
		return instance_;
	}
};

} // realm, ember