/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <nlohmann/json.hpp>

namespace ember::journal {

enum class Operation {
	shutdown,
	restart
};

enum class Status {
	pending,
	in_progress,
	complete
};

struct Entry {
	std::string service;
	std::string build;
	Operation operation;
	Status status;
	bool announce;
	std::chrono::system_clock::time_point time;
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    ember::journal::Operation,
    {
        { ember::journal::Operation::shutdown, "shutdown" },
        { ember::journal::Operation::restart,  "restart"  }
    }
)

NLOHMANN_JSON_SERIALIZE_ENUM(
    ember::journal::Status,
    {
        { ember::journal::Status::pending,     "pending"     },
        { ember::journal::Status::in_progress, "in_progress" },
        { ember::journal::Status::complete,    "complete"    }
    }
)

} // journal, ember

namespace nlohmann {

template <>
struct adl_serializer<ember::journal::Entry> {
	static ember::journal::Entry from_json(const nlohmann::json& j) {
		const auto time = j.at("time").get<std::time_t>();

		return {
			j.at("service"),
			j.at("build"),
			j.at("operation"),
			j.at("status"),
			j.at("announce"),
			std::chrono::system_clock::from_time_t(time)
		};
	}

	static void to_json(nlohmann::json& j, const ember::journal::Entry& entry) {
		j = {
			{ "service", entry.service },
			{ "build", entry.build},
			{ "operation", entry.operation },
			{ "status", entry.status},
			{ "announce", entry.announce },
			{ "time", std::chrono::system_clock::to_time_t(entry.time) }
		};
	}
};

} // namespace nlohmann