/*
 * Copyright (c) 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Types.h"
#include <string>

namespace ember::dbc {

void generate_sql_ddl(const types::Definitions& defs, const std::string& out_path);

} // dbc, ember