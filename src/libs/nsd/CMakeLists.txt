# Copyright (c) 2024 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME nsd)

set(LIBRARY_HDR
    include/nsd/NSD.h
)

set(LIBRARY_SRC
    src/NSD.cpp
)

add_library(${LIBRARY_NAME} STATIC ${LIBRARY_HDR} ${LIBRARY_SRC})

add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PUBLIC shared spark ${Boost_LIBRARIES})
set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")