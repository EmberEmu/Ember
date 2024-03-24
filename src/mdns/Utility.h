/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>

namespace ember::dns {

struct Query;
struct ResourceRecord;

std::string to_string(const Query& query);
std::string to_string(const ResourceRecord& record);

} // dns, ember