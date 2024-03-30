/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>

namespace ember {

// These all exist to help demangle strings from FlatBuffers
std::string remove_fbs_ns(const std::string& name);
std::string snake_case(const std::string& val);
std::string to_cpp_ns(const std::string& val);
std::string fbs_to_name(const std::string& name);

} // ember