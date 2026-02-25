# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME thread)

add_library(${LIBRARY_NAME} STATIC
    src/ServicePool.cpp
    src/ThreadPool.cpp
    src/Utility.cpp
    include/thread/ThreadPool.h
    include/thread/Semaphore.h
    include/thread/ServicePool.h
    include/thread/Spinlock.h
    include/thread/Utility.h
)

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PUBLIC logger ${Boost_LIBRARIES})
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")