/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

namespace ember {

struct PatchMeta;

} // ember

namespace ember::dal {

class PatchDAO {
public:
	virtual std::vector<PatchMeta> fetch_patches() const = 0;
	virtual void update(const PatchMeta& meta) const = 0;
	virtual ~PatchDAO() = default;
};

} // dal, ember