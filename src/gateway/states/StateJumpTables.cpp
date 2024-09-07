/*
* Copyright (c) 2016 - 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "StateJumpTables.h"
#include "Authentication.h"
#include "CharacterList.h"
#include "SessionClose.h"
#include "WorldForwarder.h"
#include "WorldEnter.h"
#include "WorldTransfer.h"

namespace ember { 

const JumpTable<event_handler> update_event {
	&authentication::handle_event,
	&character_list::handle_event,
	&world_enter::handle_event,
	&world_transfer::handle_event,
	&world::handle_event,
	&session_close::handle_event
};

const JumpTable<packet_handler> update_packet {
	&authentication::handle_packet,
	&character_list::handle_packet,
	&world_enter::handle_packet,
	&world_transfer::handle_packet,
	&world::handle_packet,
	&session_close::handle_packet
};

const JumpTable<state_func> exit_states {
	&authentication::exit,
	&character_list::exit,
	&world_enter::exit,
	&world_transfer::exit,
	&world::exit,
	&session_close::exit
};

const JumpTable<state_func> enter_states {
	&authentication::enter,
	&character_list::enter,
	&world_enter::enter,
	&world_transfer::enter,
	&world::enter,
	&session_close::enter
};

} // ember