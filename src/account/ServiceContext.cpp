/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ServiceContext.h"
#include "ServiceContextImpl.h"

namespace ember::account {

ServiceContext::ServiceContext()
	: impl(std::make_unique<Impl>()) {
}

void ServiceContext::reset() {
	impl.reset();
}

ServiceContext::~ServiceContext() = default;

} // account, ember