/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "BuildDefines.h"
#include "Client.h"
#include <spark/buffers/allocators/TLSBlockAllocator.h>

namespace ember::realm {

#ifdef ENABLE_PAGE_LOCKING
using PageLockPolicy = spark::io::PageLock;
#else
using PageLockPolicy = spark::io::NoPageLock;
#endif

using ClientAllocator = spark::io::TLSBlockAllocator<
	Client,
	PREALLOCATED_CLIENTS_PER_THREAD,
	spark::io::NoRefCounting,
	spark::io::UnsafeEntrant,
	PageLockPolicy
>;

} // realm, ember

