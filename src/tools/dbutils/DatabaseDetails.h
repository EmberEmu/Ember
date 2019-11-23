/*
 * Copyright (c) 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
 
#pragma once

#include <string>
#include <cstdint>

struct DatabaseDetails {
    std::string username;
    std::string password;
    std::string hostname;
    std::uint16_t port;
};

struct Migration {
    std::uint32_t id;
    std::string core_version;
    std::string install_date;
    std::string installed_by;
    std::string commit_hash;
    std::string file;
};