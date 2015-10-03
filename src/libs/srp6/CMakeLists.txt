# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(srp6
            src/Generator.cpp
            src/Util.cpp
            src/Client.cpp
            src/Server.cpp
            include/srp6/Util.h
            include/srp6/Server.h
            include/srp6/Client.h
            include/srp6/Generator.h
            include/srp6/Exception.h
           )

target_include_directories(srp6 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)