# Copyright (c) 2014, 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


set(EXECUTABLE_SRC
    srp6.cpp
    BufferChain.cpp
    Buffer.cpp
    BinaryStream.cpp
    GruntHandler.cpp
    GruntProtocol.cpp
    LoginHandler.cpp
    Patcher.cpp
    IPBan.cpp
    )

add_executable(unit_tests ${EXECUTABLE_SRC})
target_link_libraries(unit_tests gtest gtest_main liblogin shared spark srp6 ${BOTAN_LIBRARY} ${Boost_LIBRARIES})
target_include_directories(unit_tests PRIVATE ../src)
add_test(NAME unit_tests COMMAND unit_tests)