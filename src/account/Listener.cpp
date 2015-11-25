/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Listener.h"

namespace ember {

Listener::Listener(spark::Service& service) : service_(service) { 
	
}

Listener::~Listener() {

}

void Listener::handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {

}

void Listener::handle_link_event(const spark::Link& link, spark::LinkState event) {

}

} // ember