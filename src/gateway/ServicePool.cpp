/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ServicePool.h"
#include <shared/threading/Affinity.h>

namespace ember {

ServicePool::ServicePool(unsigned int size) {

}

boost::asio::io_service* ServicePool::get_service() {
	return nullptr;
}

void ServicePool::run() {

}

} // ember