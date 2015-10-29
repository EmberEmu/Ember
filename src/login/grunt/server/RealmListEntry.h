/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Packet.h"
#include "../ResultCodes.h"
#include "../../Realm.h"
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

/*
 * The parsing for this packet is pretty verbose but I'm assuming that the data
 * from the server may be invalid (unlikely but better safe than sorry)
 */

namespace ember { namespace grunt { namespace server {