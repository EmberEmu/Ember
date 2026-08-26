/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/BlazeSDK.h>
#include <memory>

namespace blazeas {

void launch(std::unique_ptr<ember::blaze::Client> client);

} // blazeas