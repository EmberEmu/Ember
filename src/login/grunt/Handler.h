/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packets.h"
#include "Exceptions.h"
#include <spark/buffers/Buffer.h>
#include <logger/Logging.h>
#include <functional>
#include <optional>
#include <variant>
#include <cstddef>

namespace ember::grunt {

class Handler final {
	enum class State {
		NEW_PACKET, READ
	};

	std::variant<
		client::LoginChallenge,
		client::LoginProof,
		client::ReconnectProof,
		client::SurveyResult,
		client::RequestRealmList,
		client::TransferAccept,
		client::TransferResume,
		client::TransferCancel
	> packet_;

	Packet* curr_packet_ = nullptr;
	State state_ = State::NEW_PACKET;

	log::Logger* logger_;

	void handle_new_packet(spark::Buffer& buffer);
	void handle_read(spark::Buffer& buffer, std::size_t offset);
	void dump_bad_packet(const spark::buffer_underrun& e,
	                     spark::Buffer& buffer,
	                     std::size_t offset);

public:
	explicit Handler(log::Logger* logger) : logger_(logger) { }

	const Packet* const try_deserialise(spark::Buffer& buffer);
};

} // grunt, ember