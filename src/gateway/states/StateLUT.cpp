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

namespace ember { 

const state_func update_states[] = {
	&authentication::update,
	&queue::update,
	&character_list::update,
	&world::update
};

const state_func exit_states[] = {
	&authentication::exit,
	&queue::exit,
	&character_list::exit,
	&world::exit
};

const state_func enter_states[] = {
	&authentication::enter,
	&queue::enter,
	&character_list::enter,
	&world::enter
};

} // ember