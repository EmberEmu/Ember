/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <span>
#include <string>
#include <vector>
#include <cstdint>

namespace ember::stun {

class TransportBase {
public:
	using ReceiveCallback = std::function<void(std::vector<std::uint8_t>)>;
	using OnConnectionError = std::function<void(const boost::system::error_code&)>;

	ReceiveCallback rcb_;
	OnConnectionError ecb_;

	virtual void connect() = 0;
	virtual void close() = 0;
	virtual void send(std::vector<std::uint8_t> message) = 0;
	virtual boost::asio::io_context* executor() = 0;
	virtual std::chrono::milliseconds timeout() = 0;
	virtual unsigned int retries() = 0;

	virtual ~TransportBase() = default;

	virtual void set_callbacks(ReceiveCallback rcb, OnConnectionError ecb) {
		if(!rcb || !ecb) {
			throw std::invalid_argument("Transport callbacks cannot be null");
		}

		rcb_ = rcb;
		ecb_ = ecb;
	}
};

} // stun, ember