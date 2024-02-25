# Copyright (c) 2014 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME unit_tests)

set(EXECUTABLE_SRC
    srp6.cpp
    DynamicBuffer.cpp
    Buffer.cpp
    BinaryStream.cpp
    GruntHandler.cpp
    GruntProtocol.cpp
    LoginHandler.cpp
    Patcher.cpp
    IPBan.cpp
    DNS.cpp
    PINAuthenticator.cpp
    UTF8Utilities.cpp
    STUN.cpp
    )

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} gtest gtest_main liblogin shared spark srp6 libmdns stun ${BOTAN_LIBRARY} ${Boost_LIBRARIES})
target_include_directories(${EXECUTABLE_NAME} PRIVATE ../src)
gtest_discover_tests(${EXECUTABLE_NAME})
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})