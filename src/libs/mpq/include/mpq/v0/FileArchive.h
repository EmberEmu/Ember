/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mpq/base/FileArchive.h>

namespace ember::mpq {

namespace v0 {

class FileArchive : public mpq::FileArchive {
public:
	FileArchive(std::filesystem::path path, std::uintptr_t offset)
		: mpq::FileArchive(path, offset) {}
};

}

} // mpq, ember