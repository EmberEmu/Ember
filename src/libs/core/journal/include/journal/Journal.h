/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <journal/JournalEntry.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace ember::journal {

class Journal {
	std::string service_;
	std::string version_;
	std::filesystem::path path_;

	Entry retrieve(std::istream& is);
	bool write(const Entry& entry);

public:
	Journal(std::filesystem::path path, std::string service, std::string version);

	std::optional<Entry> retrieve();
	bool create(const std::chrono::system_clock::time_point& time, Operation operation, bool announce);
	bool update(Status status);
	bool remove();
};

} // journal, ember