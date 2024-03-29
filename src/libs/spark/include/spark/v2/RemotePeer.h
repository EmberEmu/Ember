/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/PeerHandler.h>
#include <spark/v2/PeerConnection.h>
#include <spark/v2/Dispatcher.h>
#include <spark/v2/SharedDefs.h>
#include <logger/Logging.h>
#include <concepts>
#include <utility>

namespace ember::spark::v2 {

template<typename Handler, typename Connection, typename Socket>
class RemotePeer final : public Dispatcher {
	Handler handler_;
	Connection conn_;
	Mode mode_;
	log::Logger* log_;

public:
	explicit RemotePeer(Socket socket, Mode mode, log::Logger* log)
		: handler_(*this), mode_(mode), conn_(*this, std::move(socket)), log_(log) {
		if(mode_ == Mode::SERVER) {
			initiate_hello();
		}
	}

	void initiate_hello() {

	}

	void send() {
		LOG_TRACE(log_) << __func__ << LOG_ASYNC;
	}

	void receive(std::span<const std::uint8_t> data) override {
		LOG_TRACE(log_) << __func__ << LOG_ASYNC;
	}
};

} // spark, ember