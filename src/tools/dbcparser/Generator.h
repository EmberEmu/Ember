/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Types.h"
#include <vector>
#include <string>

namespace ember::dbc {

void generate_common(const types::Definitions& defs, const std::string& output, const std::string& template_path);
void generate_disk_source(const types::Definitions& defs, const std::string& output, const std::string& template_path);

} // dbc, ember