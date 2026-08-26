/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ember/blaze/BlazeSDK.h>
#include <ember/blaze/impl/Client.h>

namespace ember::blaze {

Client::Client() : impl_(std::make_unique<impl::Client>()) {}

Client::~Client() {

}

} // blaze, ember