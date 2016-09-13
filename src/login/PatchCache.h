/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PatchMeta.h"
#include <vector>
#include <string>

namespace ember { namespace patch_cache {

std::vector<PatchMeta> fetch(const std::string& path);

}} // patch_cache, ember