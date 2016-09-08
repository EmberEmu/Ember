/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StateLUT.h"
#include "Authentication.h"
#include "CharacterList.h"
#include "WorldForwarder.h"
#include "InQueue.h"
#include "SessionClose.h"

namespace ember { 

const event_handler update_event[] = {
    &authentication::handle_event,
    &queue::handle_event,
    &character_list::handle_event,
    &world::handle_event,
    &session_close::handle_event
};

const state_func update_packet[] = {
	&authentication::handle_packet,
	&queue::handle_packet,
	&character_list::handle_packet,
	&world::handle_packet,
	&session_close::handle_packet
};

const state_func exit_states[] = {
	&authentication::exit,
	&queue::exit,
	&character_list::exit,
	&world::exit,
	&session_close::exit
};

const state_func enter_states[] = {
	&authentication::enter,
	&queue::enter,
	&character_list::enter,
	&world::enter,
	&session_close::enter
};

} // ember