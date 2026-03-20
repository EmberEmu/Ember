/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packets.h"
#include <spark/buffers/pmr/Buffer.h>
#include <logger/LoggerFwd.h>
#include <functional>
#include <optional>
#include <type_traits>
#include <variant>
#include <cstddef>

namespace ember::grunt {

class Handler final {
	enum class State {
		new_packet,
		read
	};

	std::variant<
		std::monostate,
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
	State state_ = State::new_packet;

	log::Logger& logger_;

	template<typename T> void create_packet();
	void handle_new_packet(spark::io::pmr::Buffer& buffer);
	void handle_read(spark::io::pmr::Buffer& buffer, std::size_t offset);
	void dump_bad_packet(const spark::io::buffer_underrun& e,
	                     spark::io::pmr::Buffer& buffer,
	                     std::size_t offset);

public:
	explicit Handler(log::Logger& logger) : logger_(logger) { }

	const Packet* process_buffer(spark::io::pmr::Buffer& buffer);
};

} // grunt, ember