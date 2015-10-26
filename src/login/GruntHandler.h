/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketHandler.h"

namespace ember {

class GruntHandler final : public PacketHandler {
public:
	void handle_packet(spark::Buffer& buffer) override;
};

} // ember