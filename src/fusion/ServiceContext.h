/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Library.h"
#include "Prototypes.h"
#include <logger/Logger.h>
#include <service/Service.h>

namespace ember::fusion {

struct ServiceContext {
	IService* service;
	library::Handle lib_handle;
	std::unique_ptr<log::Logger> logger;
	ServiceIndex index;
};

} // fusion, ember