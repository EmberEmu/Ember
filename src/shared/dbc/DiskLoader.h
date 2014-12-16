/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Loader.h"
#include "Storage.h"
#include <memory>
#include <string>

namespace ember { namespace dbc {

class DiskLoader final : public Loader {
	const std::string dir_path_;

	Storage load() const;

public:
	DiskLoader(std::string dir_path) : dir_path_(std::move(dir_path)) {}
	~DiskLoader() override {}
	Storage disk_representations() const override;
};

}} //dbc, ember