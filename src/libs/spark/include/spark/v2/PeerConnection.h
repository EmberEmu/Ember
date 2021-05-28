/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Dispatcher.h>
#include <concepts>

namespace ember::spark::v2 {

template<std::movable Socket>
class PeerConnection final {
	Dispatcher& dispatcher_;
	Socket socket_;

public:
	PeerConnection(Dispatcher& dispatcher, Socket socket)
		: dispatcher_(dispatcher), socket_(std::move(socket)) {}
	PeerConnection(PeerConnection&&) = default;
};

} // spark, ember