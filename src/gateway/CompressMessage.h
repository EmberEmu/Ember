/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Packets.h>
#include <spark/buffers/Buffer.h>

namespace ember {

template<typename Packet_t>
int compress_message(const Packet_t& packet, spark::Buffer& out, int compression_level);
int compress_message(const spark::Buffer& in, spark::Buffer& out, int compression_level);

} // ember