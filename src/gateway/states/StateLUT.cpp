/*
* Copyright (c) 2016 - 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "StateLUT.h"
#include "Authentication.h"
#include "CharacterList.h"
#include "WorldForwarder.h"
#include "WorldEnter.h"
#include "SessionClose.h"
#include <protocol/Opcodes.h>

namespace ember { 

const event_handler update_event[] = {
	&authentication::handle_event,
	&character_list::handle_event,
	&world_enter::handle_event,
	nullptr, // world transfer, unhandled
	&world::handle_event,
	&session_close::handle_event
};

const packet_handler update_packet[] = {
	&authentication::handle_packet,
	&character_list::handle_packet,
	&world_enter::handle_packet,
	nullptr, // world transfer, unhandled
	&world::handle_packet,
	&session_close::handle_packet
};

const state_func exit_states[] = {
	&authentication::exit,
	&character_list::exit,
	&world_enter::exit,
	nullptr, // world transfer, unhandled
	&world::exit,
	&session_close::exit
};

const state_func enter_states[] = {
	&authentication::enter,
	&character_list::enter,
	&world_enter::enter,
	nullptr, // world transfer, unhandled
	&world::enter,
	&session_close::enter
};

} // ember