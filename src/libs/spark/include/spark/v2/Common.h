/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <flatbuffers/flatbuffer_builder.h>
#include <boost/container/small_vector.hpp>
#include <expected>
#include <functional>
#include <span>
#include <cstdint>

namespace ember::spark::v2 {

struct Link;

struct Message {
	boost::container::small_vector<std::uint8_t, 1000> header; // temp
	flatbuffers::FlatBufferBuilder fbb;
};

using MessageCB = std::function<void()>;
using TrackedHandler = std::function<void(const Link&, std::expected<bool, Message>)>;

} // v2, spark, ember