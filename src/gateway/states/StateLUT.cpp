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

const enter_state update_states[] = {
	&authentication::update,
	&queue::update,
	&character_list::update,
	&world::update
};

const exit_state exit_states[] = {
	&authentication::exit,
	&queue::exit,
	&character_list::exit,
	&world::exit
};

const update_state enter_states[] = {
	&authentication::enter,
	&queue::enter,
	&character_list::enter,
	&world::enter
};

} // ember