# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME taskmaster)

add_library(${LIBRARY_NAME}
            include/Task.h
            include/Scheduler.h
            src/Scheduler.cpp
           )

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)