/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <dbcreader/Loader.h>
#include <dbcreader/Storage.h>
#include <initializer_list>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace ember::dbc {

class DiskLoader final : public Loader {
	typedef std::function<void(Storage&, const std::string&)> DBCLoadFunc;
	typedef std::function<void(const std::string&)> LogCB;

	const LogCB log_cb_;
	const std::string dir_path_;
	std::unordered_map<std::string, DBCLoadFunc> dbc_map;

public:
	DiskLoader(std::string dir_path, LogCB log_cb = [](const std::string&){});
	~DiskLoader() = default;

	Storage load() const override;
	Storage load(std::initializer_list<const std::string> whitelist) const override;
	Storage load(std::span<const std::string> whitelist) const override;
};

} // dbc, ember