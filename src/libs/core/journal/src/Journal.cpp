/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <journal/Journal.h>
#include "nlohmann/json.hpp"
#include <fstream>
#include <utility>
#include <stdexcept>

namespace ember::journal {

constexpr auto static json_indentation = 4;

Journal::Journal(std::filesystem::path path, std::string service, std::string version)
	: service_(std::move(service))
	, version_(std::move(version))
	, path_(std::move(path)) {
}

Entry Journal::retrieve(std::istream& is) {
	return nlohmann::json::parse(is).get<Entry>();
}

std::optional<Entry> Journal::retrieve() {
	if(!std::filesystem::exists(path_) || std::filesystem::is_empty(path_)) {
		return std::nullopt;
	}

	std::ifstream ifs(path_);

	if(!ifs) {
		throw std::runtime_error("Unable to open journal file for reading");
	}

	return retrieve(ifs);
}

bool Journal::create(const std::chrono::system_clock::time_point& time, Operation operation, bool announce) {
	const Entry entry {
		.service = service_,
		.build = version_,
		.operation = operation,
		.status = Status::pending,
		.announce = announce,
		.time = time
	};

	return write(entry);
}

bool Journal::update(Status status) {
	auto entry = retrieve();

	if(!entry) {
		return false;
	}

	entry->status = status;
	return write(*entry);
}

bool Journal::write(const Entry& entry) {
	std::filesystem::path tmp(path_.string() + ".tmp");

	{
		std::ofstream ofs(tmp);

		if(!ofs) {
			return false;
		}

		ofs << nlohmann::json(entry).dump(json_indentation);

		if(!ofs) {
			return false;
		}
	}

	std::error_code ec;
	std::filesystem::rename(tmp, path_, ec);

	if(ec) {
		std::filesystem::remove(tmp);
		return false;
	}

	return true;
}

bool Journal::remove() {
	return std::filesystem::remove(path_);
}

} // journal, ember