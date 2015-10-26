/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <overseer/Overseer.h>

namespace ember { namespace overseer {

Overseer::Overseer(unsigned int thread_count)
                  : thread_count_(thread_count) { }

Overseer::~Overseer() {
	stop();
}

void Overseer::stop() {
	for(auto& worker : workers_) {
		if(worker.joinable()) {
			worker.join();
		}
	}
}

}} // overseer, ember