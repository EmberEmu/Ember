# Copyright (c) 2015 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME srp6)

add_library(${LIBRARY_NAME} STATIC
    src/Generator.cpp
    src/Utility.cpp
    src/Client.cpp
    src/Server.cpp
    include/srp6/Utility.h
    include/srp6/Server.h
    include/srp6/Client.h
    include/srp6/Generator.h
    include/srp6/Exception.h
    include/srp6/detail/Primes.h
)

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PUBLIC botan::botan PRIVATE ${Boost_LIBRARIES})
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")
