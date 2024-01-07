# Copyright (c) 2021 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME dnssd)

add_library(${LIBRARY_NAME}
    include/dnssd/DNSSD.h
)

add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(dnssd PROPERTIES FOLDER "Libraries")