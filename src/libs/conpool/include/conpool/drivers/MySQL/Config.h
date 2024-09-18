/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/MySQL/Driver.h>
#include <string>
#include <string_view>

namespace ember::drivers {

ember::drivers::MySQL init_db_driver(const std::string& config_path, std::string_view section);

} // drivers, ember