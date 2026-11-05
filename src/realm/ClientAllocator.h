/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Config.h"
#include "Client.h"
#include <allocators/BlockAllocator.h>

namespace ember::realm {

#ifdef ENABLE_PAGE_LOCKING
using PageLockPolicy = allocators::PageLock;
#else
using PageLockPolicy = allocators::NoPageLock;
#endif

using ClientAllocator = allocators::BlockAllocator<
	Client,
	PageLockPolicy,
	allocators::NoValidateDealloc
>;

} // realm, ember