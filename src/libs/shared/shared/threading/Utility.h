/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <thread>

namespace ember::thread {

void set_affinity(std::thread& thread, unsigned int core);

void set_name(const char* name);
void set_name(std::thread& thread, const char* name);

std::wstring get_name(std::thread& thread);
std::wstring get_name();

} // thread, ember
