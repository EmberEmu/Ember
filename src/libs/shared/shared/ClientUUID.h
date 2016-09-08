/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <cstdint>

namespace ember { namespace client_uuid {

struct uuid {
	union {
		std::uint8_t data[16];

		struct {
			std::uint8_t service;
			std::uint8_t rand[15];
		};
	};
};

inline uuid from_bytes(const std::uint8_t* data) {
	uuid uuid;
	std::copy(data, data + 16, uuid.data);
	return uuid;
}

inline uuid generate(std::size_t service_index) {
	uuid uuid;

	for(std::size_t i = 0; i < sizeof(uuid); ++i) {
		uuid[i] = 1; // todo
	}

	uuid.service = service_index;
	return uuid;
}

}} // client_uuid, ember