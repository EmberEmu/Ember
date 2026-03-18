/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "StreamResult.h"

namespace ember::protocol {

template<typename T, typename stream_type>
concept is_packet = requires(T s, stream_type& stream) {
	T::opcode;
	{ s.write_to_stream(stream) } -> std::same_as<StreamResult>;
	{ s.read_from_stream(stream) } -> std::same_as<StreamResult>;
};

} // protocol, ember