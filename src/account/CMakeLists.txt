# Copyright (c) 2015 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME account)
set(LIBRARY_NAME    libaccount)

set(LIBRARY_HDR
    FilterTypes.h
    AccountService.h
    Sessions.h
    AccountHandler.h
    Service.h
)

set(LIBRARY_SRC
    AccountService.cpp
    Sessions.cpp
    AccountHandler.cpp
    Service.cpp
)

include_directories(${SPARK_INCLUDES_DIR})
add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_link_libraries(${LIBRARY_NAME} PUBLIC logger shared ${Boost_LIBRARIES} PRIVATE spark conpool ${MYSQLCCPP_LIBRARY} Threads::Threads)

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} logger shared ${Boost_LIBRARIES} Threads::Threads)

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(account PROPERTIES FOLDER "Services/Applications")
set_target_properties(libaccount PROPERTIES FOLDER "Services/Libraries")
