/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Client.h>
#include <functional>
#include <cstdint>
#include <queue>

namespace ember::ports {

class Daemon {
	using ResultHandler = std::function<void(bool)>;
	
	struct Mapping {
		std::uint16_t internal;
		std::uint16_t external;
		std::uint32_t lifetime;
	};

	struct Request {
		ResultHandler handler;
		
		enum class Op {
			OPEN_MAPPING, DELETE_MAPPING
		} op_;
	};

	Client& client_;
	boost::asio::io_context& ctx_;
	std::queue<Request> tasks_;
	std::vector<Mapping> active_maps_;

public:
	Daemon(Client& client, boost::asio::io_context& ctx)
		: client_(client), ctx_(ctx) {}
	~Daemon();

	void add_mapping(std::uint16_t internal, std::uint16_t external,
	                std::uint32_t lifetime, ResultHandler&& handler);
	void delete_mapping(std::uint16_t internal, std::uint16_t external,
	                    ResultHandler&& handler);
};

} // natpmp, ports, ember
