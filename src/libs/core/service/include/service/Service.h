/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <service/Config.h>

namespace ember {

class EMBER_EXPORT_SERVICE IService {
public:
	IService() = default;
	virtual ~IService() = default;

	virtual void stop() = 0;
};

} // ember