/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Common.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <spark/v2/buffers/BinaryStream.h>

namespace ember::spark::v2 {

static void write_header(Message& msg) {
	MessageHeader header;
	header.size = msg.fbb.GetSize();
	header.set_alignment(msg.fbb.GetBufferMinAlignment());

	BufferAdaptor adaptor(msg.header);
	BinaryStream stream(adaptor);
	header.write_to_stream(stream);
}

template<typename T>
static void finish(T& payload, Message& msg) {
	core::HeaderT header_t;
	core::MessageUnion mu;
	mu.Set(payload);
	header_t.message = mu;
	msg.fbb.Finish(core::Header::Pack(msg.fbb, &header_t));
}


} // spark, ember